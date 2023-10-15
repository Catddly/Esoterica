#include "RenderPipelineRegistry.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceRequesterID.h"
#include "Base/Threading/Threading.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/RHI/RHIDevice.h"

namespace EE::Render
{
	PipelineHandle::PipelineHandle( PipelineType type, uint32_t id )
		: m_ID(id), m_type( type )
	{}

	PipelineRegistry::~PipelineRegistry()
	{
		EE_ASSERT( m_rasterPipelineStatesCache.empty() );
		EE_ASSERT( m_rasterPipelineHandlesCache.empty() );
	}

    //-------------------------------------------------------------------------

	void PipelineRegistry::Initialize( SystemRegistry const& systemRegistry )
	{
		EE_ASSERT( !m_isInitialized );

        //m_pTaskSystem = systemRegistry.GetSystem<TaskSystem>();
		m_pResourceSystem = systemRegistry.GetSystem<Resource::ResourceSystem>();

		EE_ASSERT( m_pResourceSystem != nullptr );

		m_isInitialized = true;
	}

	void PipelineRegistry::Shutdown()
	{
		EE_ASSERT( m_isInitialized );

		UnloadAllPipelineShaders();

        // check if the RHI resources is already release
        for ( auto const& entry : m_rasterPipelineStatesCache )
        {
            if ( entry->IsVisible() )
            {
                EE_LOG_ERROR( "Render", "PipelineRegistry::Shutdown", "Pipeline states not clear up. Did you forget to call DestroyAllPipelineState()?" );
                EE_ASSERT( false );
            }
        }

        m_rasterPipelineStatesCache.clear();
        m_rasterPipelineHandlesCache.clear();

		m_pResourceSystem->WaitForAllRequestsToComplete();
		m_isInitialized = false;
	}

	//-------------------------------------------------------------------------

	PipelineHandle PipelineRegistry::RegisterRasterPipeline( RHI::RHIRasterPipelineStateCreateDesc const& rasterPipelineDesc )
	{
		EE_ASSERT( Threading::IsMainThread() );
		EE_ASSERT( m_isInitialized );
		EE_ASSERT( rasterPipelineDesc.IsValid() );

		// Already exists, immediately return
		//-------------------------------------------------------------------------

		auto iter = m_rasterPipelineHandlesCache.find( rasterPipelineDesc );
		if ( iter != m_rasterPipelineHandlesCache.end() )
		{
			return iter->second;
		}

		// Not exists, create new entry
		//-------------------------------------------------------------------------

		auto nextId = static_cast<uint32_t>( m_rasterPipelineStatesCache.size() ) + 1;
		PipelineHandle newHandle = PipelineHandle( PipelineType::Raster, nextId );

		auto pEntry = MakeShared<RasterPipelineEntry>();
		if ( pEntry )
		{
			pEntry->m_handle = newHandle;
            pEntry->m_desc = rasterPipelineDesc;

			for ( auto const& piplineShader : rasterPipelineDesc.m_pipelineShaders )
			{
				switch ( piplineShader.m_stage )
				{
					case PipelineStage::Vertex:
					{
						pEntry->m_vertexShader = ResourceID( piplineShader.m_shaderPath );
						EE_ASSERT( pEntry->m_vertexShader.IsSet() );
						break;
					}
					case PipelineStage::Pixel:
					{
						pEntry->m_pixelShader = ResourceID( piplineShader.m_shaderPath );
						EE_ASSERT( pEntry->m_pixelShader.IsSet() );
						break;
					}
					case PipelineStage::Compute:
					{
						EE_LOG_ERROR( "Render", "Render Pipeline Registry", "Registering a raster pipeline, but a compute shader was found!" );
						EE_HALT();
						break;
					}
					default:
					{
						EE_LOG_WARNING( "Render", "Render Pipeline Registry", "Try to registering a not support raster pipeline shader!" );
						break;
					}
				}
			}

			m_rasterPipelineStatesCache.Add( pEntry );
			m_rasterPipelineHandlesCache.insert( { rasterPipelineDesc, newHandle } );

			EE_ASSERT( m_rasterPipelineStatesCache.size() == m_rasterPipelineHandlesCache.size() );

			m_waitToSubmitRasterPipelines.emplace_back( pEntry );

			return newHandle;
		}

		return PipelineHandle();
	}

	PipelineHandle PipelineRegistry::RegisterComputePipeline( ComputePipelineDesc const& computePipelineDesc )
	{
		EE_ASSERT( Threading::IsMainThread() );
		EE_ASSERT( m_isInitialized );

		return PipelineHandle();
	}

	void PipelineRegistry::LoadAndUpdatePipelines( RHI::RHIDevice* pDevice )
	{
		EE_ASSERT( Threading::IsMainThread() );
		EE_ASSERT( m_isInitialized );
        EE_ASSERT( pDevice != nullptr );

        // Update loading
        //-------------------------------------------------------------------------

		UpdateLoadPipelineShaders();

        // Registered loaded pipelines to create actual RHI resources
        //-------------------------------------------------------------------------

        for ( auto& rasterEntry : m_waitToRegisteredRasterPipelines )
        {
            // double checked again in case pipeline entry is unloaded by chance.
            if ( rasterEntry->IsReadyToCreatePipelineLayout() )
            {
                if ( !TryCreateRHIRasterPipelineStateForEntry( rasterEntry, pDevice ) )
                {
                    m_retryRasterPipelineCaches.push_back( rasterEntry );
                }
            }
        }

        if ( !m_waitToRegisteredRasterPipelines.empty() )
        {
            m_waitToRegisteredRasterPipelines.clear();
            std::swap( m_waitToRegisteredRasterPipelines, m_retryRasterPipelineCaches );
        }
	}

    void PipelineRegistry::DestroyAllPipelineState( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );

        EE_ASSERT( m_rasterPipelineStatesCache.size() == m_rasterPipelineHandlesCache.size() );
        for ( auto& entry : m_rasterPipelineStatesCache )
        {
            if ( entry->IsVisible() )
            {
                pDevice->DestroyRasterPipelineState( entry->m_pPipelineState );
                entry->m_pPipelineState = nullptr;
            }
        }

        // Note: can NOT call m_rasterPipelineStatesCache here, because entry contains TResourcePtr.
        // We need to unload it first before we destroy the whole entry.
    }

	//-------------------------------------------------------------------------

	void PipelineRegistry::UpdateLoadPipelineShaders()
	{
		EE_ASSERT( m_pResourceSystem != nullptr );

		// Submit raster pipeline load tasks
		//-------------------------------------------------------------------------

		if ( !m_waitToSubmitRasterPipelines.empty() )
		{
			for ( uint32_t i = 0; i < m_waitToSubmitRasterPipelines.size(); ++i )
			{
				auto const& pEntry = m_waitToSubmitRasterPipelines[i];

				if ( pEntry->m_vertexShader.IsSet() )
				{
					m_pResourceSystem->LoadResource( pEntry->m_vertexShader, Resource::ResourceRequesterID( pEntry->GetID().m_ID ) );
				}

				if ( pEntry->m_pixelShader.IsSet() )
				{
					m_pResourceSystem->LoadResource( pEntry->m_pixelShader, Resource::ResourceRequesterID( pEntry->GetID().m_ID ) );
				}

				m_waitToLoadRasterPipelines.push_back( pEntry );
			}

			m_waitToSubmitRasterPipelines.clear();
		}

		// Update shader load status
		//-------------------------------------------------------------------------

		//if ( false && m_pTaskSystem ) // multi-threading
		//{
		//	EE_UNIMPLEMENTED_FUNCTION();
		//	struct PipelineShaderLoadTask final : ITaskSet
		//	{
		//		PipelineShaderLoadTask( TVector<TSharedPtr<RasterPipelineEntry>>& rasterPipelineEntries )
		//			: m_waitToLoadRasterPipelines( rasterPipelineEntries )
		//		{
		//			m_SetSize = static_cast<uint32_t>( rasterPipelineEntries.size() );
		//		}

		//		virtual void ExecuteRange( TaskSetPartition range, uint32_t threadnum ) override final
		//		{
		//			for ( uint64_t i = range.start; i < range.end; ++i )
		//			{
		//				//auto pEntity = m_updateList[i];

		//				//// Ignore any entities with spatial parents, these will be updated by their parents
		//				//if ( pEntity->HasSpatialParent() )
		//				//{
		//				//	continue;
		//				//}

		//				////-------------------------------------------------------------------------

		//				//if ( pEntity->HasAttachedEntities() )
		//				//{
		//				//	EE_PROFILE_SCOPE_ENTITY( "Update Entity Chain" );
		//				//	RecursiveEntityUpdate( pEntity );
		//				//}
		//				//else // Direct entity update
		//				//{
		//				//	EE_PROFILE_SCOPE_ENTITY( "Update Entity" );
		//				//	pEntity->UpdateSystems( m_context );
		//				//}
		//			}
		//		}

		//	private:

		//		TVector<TSharedPtr<RasterPipelineEntry>>& m_waitToLoadRasterPipelines;
		//	};
		//}

        // Update all loading pipelines and check to see if loading has success
        //-------------------------------------------------------------------------

		if ( !m_waitToLoadRasterPipelines.empty() )
		{
			for ( auto beg = m_waitToLoadRasterPipelines.begin(); beg != m_waitToLoadRasterPipelines.end(); ++beg )
			{
				auto const& pEntry = *beg;

                // TODO: If not successfully loaded? check HasLoadFailed().
				if ( pEntry->IsReadyToCreatePipelineLayout() )
				{
					if ( m_waitToLoadRasterPipelines.size() == 1 )
					{
                        m_waitToRegisteredRasterPipelines.push_back( pEntry );
						m_waitToLoadRasterPipelines.clear();
						break;
					}
					else
					{
                        m_waitToRegisteredRasterPipelines.push_back( pEntry );
						beg = m_waitToLoadRasterPipelines.erase_unsorted( beg );
					}
				}
			}
		}
	}

	void PipelineRegistry::UnloadAllPipelineShaders()
	{
		EE_ASSERT( m_pResourceSystem );

		EE_ASSERT( m_rasterPipelineStatesCache.size() == m_rasterPipelineHandlesCache.size() );
		for ( auto const& pPipeline : m_rasterPipelineStatesCache )
		{
			if ( pPipeline->m_vertexShader.IsLoaded() )
			{
				m_pResourceSystem->UnloadResource( pPipeline->m_vertexShader, Resource::ResourceRequesterID( pPipeline->GetID().m_ID ) );
			}

			if ( pPipeline->m_pixelShader.IsLoaded() )
			{
				m_pResourceSystem->UnloadResource( pPipeline->m_pixelShader, Resource::ResourceRequesterID( pPipeline->GetID().m_ID ) );
			}
		}
	}

    bool PipelineRegistry::TryCreateRHIRasterPipelineStateForEntry( TSharedPtr<RasterPipelineEntry>& rasterEntry, RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( !rasterEntry->IsVisible() );
        EE_ASSERT( Threading::IsMainThread() );

        // Safety: We make sure raster pipeline state layout will only be created by single thread,
        //         and it ResourcePtr is loaded and will not be changed by RHIDevice.
        RHI::RHIDevice::CompiledShaderArray compiledShaders;
        compiledShaders.push_back( rasterEntry->m_vertexShader.GetPtr() );
        compiledShaders.push_back( rasterEntry->m_pixelShader.GetPtr() );
        auto* pPipelineState = pDevice->CreateRasterPipelineState( rasterEntry->m_desc, compiledShaders );
        if ( pPipelineState )
        {
            rasterEntry->m_pPipelineState = pPipelineState;

            // TODO: add render graph debug name
            EE_LOG_MESSAGE( "Render", "PipelineRegistry", "[%ull] Pipeline Visible.", rasterEntry->m_desc.GetHashCode() );

            return true;
        }

        return false;
    }

    bool PipelineRegistry::IsPipelineReady( PipelineHandle const& pipelineHandle ) const
    {
        if ( pipelineHandle.IsValid() )
        {
            switch ( pipelineHandle.m_type )
            {
                case PipelineType::Raster:
                {
                    auto const& rasterEntry = m_rasterPipelineStatesCache.Get( pipelineHandle );
                    return ( *rasterEntry )->IsVisible();
                }
                case PipelineType::Compute:
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                    return false;
                }
                case PipelineType::Transfer:
                case PipelineType::RayTracing:
                default:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
            }
        }

        return false;
    }

    EE::RHI::RHIPipelineState* PipelineRegistry::GetPipeline( PipelineHandle const& pipelineHandle ) const
    {
        if ( pipelineHandle.IsValid() )
        {
            switch ( pipelineHandle.m_type )
            {
                case PipelineType::Raster:
                {
                    auto& rasterEntry = *m_rasterPipelineStatesCache.Get( pipelineHandle );
                    if ( rasterEntry->IsVisible() )
                    {
                        return rasterEntry->m_pPipelineState;
                    }

                    break;
                }
                case PipelineType::Compute:
                {
                    EE_UNIMPLEMENTED_FUNCTION();

                    break;
                }
                case PipelineType::Transfer:
                case PipelineType::RayTracing:
                default:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
            }
        }

        return nullptr;
    }

}

