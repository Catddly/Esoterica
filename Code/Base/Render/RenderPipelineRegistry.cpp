#include "RenderPipelineRegistry.h"
#include "Base/Logging/Log.h"
#include "Base/Time/Timers.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/Resource/ResourceRequesterID.h"
#include "Base/Threading/Threading.h"
#include "Base/Threading/TaskSystem.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/Network/NetworkSystem.h"

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

	bool PipelineRegistry::Initialize( Resource::ResourceSystem* pResourceSystem )
	{
		EE_ASSERT( !m_isInitialized );
		m_pResourceSystem = pResourceSystem;
		m_isInitialized = true;
        return true;
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
                        if ( pEntry->m_vertexShader.IsSet() )
                        {
                            m_pResourceSystem->LoadResource( pEntry->m_vertexShader, Resource::ResourceRequesterID( pEntry->GetID().m_ID ) );
                        }
						break;
					}
					case PipelineStage::Pixel:
					{
						pEntry->m_pixelShader = ResourceID( piplineShader.m_shaderPath );
						EE_ASSERT( pEntry->m_pixelShader.IsSet() );
                        if ( pEntry->m_pixelShader.IsSet() )
                        {
                            m_pResourceSystem->LoadResource( pEntry->m_pixelShader, Resource::ResourceRequesterID( pEntry->GetID().m_ID ) );
                        }
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

			m_waitToLoadRasterPipelines.emplace_back( pEntry );

			return newHandle;
		}

		return PipelineHandle();
	}

	PipelineHandle PipelineRegistry::RegisterComputePipeline( RHI::RHIComputePipelineStateCreateDesc const& computePipelineDesc )
	{
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );
        EE_ASSERT( computePipelineDesc.IsValid() );

        // Already exists, immediately return
        //-------------------------------------------------------------------------

        auto iter = m_computePipelineHandlesCache.find( computePipelineDesc );
        if ( iter != m_computePipelineHandlesCache.end() )
        {
            return iter->second;
        }

        // Not exists, create new entry
        //-------------------------------------------------------------------------

        auto nextId = static_cast<uint32_t>( m_computePipelineHandlesCache.size() ) + 1;
        PipelineHandle newHandle = PipelineHandle( PipelineType::Compute, nextId );

        auto pEntry = MakeShared<ComputePipelineEntry>();
        if ( pEntry )
        {
            pEntry->m_handle = newHandle;
            pEntry->m_desc = computePipelineDesc;

            EE_ASSERT( computePipelineDesc.m_pipelineShader.m_stage == Render::PipelineStage::Compute );

            pEntry->m_computeShader = ResourceID( computePipelineDesc.m_pipelineShader.m_shaderPath );
            EE_ASSERT( pEntry->m_computeShader.IsSet() );
            if ( pEntry->m_computeShader.IsSet() )
            {
                m_pResourceSystem->LoadResource( pEntry->m_computeShader, Resource::ResourceRequesterID( pEntry->GetID().m_ID ) );
            }

            m_computePipelineStatesCache.Add( pEntry );
            m_computePipelineHandlesCache.insert( { computePipelineDesc, newHandle } );

            EE_ASSERT( m_computePipelineStatesCache.size() == m_computePipelineHandlesCache.size() );

            m_waitToLoadComputePipelines.emplace_back( pEntry );

            return newHandle;
        }

        return PipelineHandle();
	}

    bool PipelineRegistry::UpdateBlock( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );
        EE_ASSERT( pDevice != nullptr );

        if ( m_pResourceSystem )
        {
            Timer<PlatformClock> updateLoopTimer;

            // TODO: async mode. Now force all shaders loaded before this frame drawing started.
            //       This can prevent some problems brought by the latency. (e.g. some render graph node
            //       wants to execute only once at the frame start, but render graph can NOT execute the commands because
            //       shader is NOT loaded yet. User thought the node is successfully executed and do NOT add it in the next frame.
            //       The message is lost forever and none of them get the valid results. )
            updateLoopTimer.Start();
            while ( !AreAllRequestedPipelineLoaded() )
            {
                Network::NetworkSystem::Update();
                m_pResourceSystem->Update();
                UpdateLoadedPipelineShaders();

                Seconds elapsedTime = updateLoopTimer.GetElapsedTimeSeconds();

                if ( elapsedTime > 5.0f )
                {
                    EE_LOG_WARNING( "Render", "Pipeline Registry", "Very long shader compile time (more than %.3f seconds), force to continue...", 5.0f );
                    break;
                }
            }

            // TODO: when pipeline registry failed to update pipelines, use old pipelines
            EE_ASSERT( TryCreatePipelineForLoadedPipelineShaders( pDevice ) );
        }

        return true;
    }

    void PipelineRegistry::DestroyAllPipelineStates( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );

        EE_ASSERT( m_rasterPipelineStatesCache.size() == m_rasterPipelineHandlesCache.size() );
        for ( auto& entry : m_rasterPipelineStatesCache )
        {
            if ( entry->IsVisible() )
            {
                pDevice->DestroyRasterPipeline( entry->m_pPipelineState );
                entry->m_pPipelineState = nullptr;
            }
        }

        // Note: can NOT clear m_rasterPipelineStatesCache here, because entry contains TResourcePtr.
        // We need to unload it first before we destroy the whole entry.

        EE_ASSERT( m_computePipelineStatesCache.size() == m_computePipelineHandlesCache.size() );
        for ( auto& entry : m_computePipelineStatesCache )
        {
            if ( entry->IsVisible() )
            {
                pDevice->DestroyComputePipeline( entry->m_pPipelineState );
                entry->m_pPipelineState = nullptr;
            }
        }
    }

	//-------------------------------------------------------------------------

    bool PipelineRegistry::AreAllRequestedPipelineLoaded() const
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );

        return m_waitToLoadRasterPipelines.empty() && m_waitToLoadComputePipelines.empty();
    }

    void PipelineRegistry::UpdateLoadedPipelineShaders()
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );

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

        if ( !m_waitToLoadComputePipelines.empty() )
        {
            for ( auto beg = m_waitToLoadComputePipelines.begin(); beg != m_waitToLoadComputePipelines.end(); ++beg )
            {
                auto const& pEntry = *beg;

                // TODO: If not successfully loaded? check HasLoadFailed().
                if ( pEntry->IsReadyToCreatePipelineLayout() )
                {
                    if ( m_waitToLoadComputePipelines.size() == 1 )
                    {
                        m_waitToRegisteredComputePipelines.push_back( pEntry );
                        m_waitToLoadComputePipelines.clear();
                        break;
                    }
                    else
                    {
                        m_waitToRegisteredComputePipelines.push_back( pEntry );
                        beg = m_waitToLoadComputePipelines.erase_unsorted( beg );
                    }
                }
            }
        }
    }

    bool PipelineRegistry::TryCreatePipelineForLoadedPipelineShaders( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_isInitialized );
        EE_ASSERT( pDevice != nullptr );

        bool bHasFailure = false;

        for ( auto& rasterEntry : m_waitToRegisteredRasterPipelines )
        {
            // double checked again in case pipeline entry is unloaded by chance.
            if ( rasterEntry->IsReadyToCreatePipelineLayout() )
            {
                if ( !TryCreateRHIRasterPipelineStateForEntry( rasterEntry, pDevice ) )
                {
                    m_retryRasterPipelineCaches.push_back( rasterEntry );
                    bHasFailure = true;
                }
            }
        }

        if ( !m_waitToRegisteredRasterPipelines.empty() )
        {
            m_waitToRegisteredRasterPipelines.clear();
            std::swap( m_waitToRegisteredRasterPipelines, m_retryRasterPipelineCaches );
        }

        //-------------------------------------------------------------------------

        for ( auto& computeEntry : m_waitToRegisteredComputePipelines )
        {
            // double checked again in case pipeline entry is unloaded by chance.
            if ( computeEntry->IsReadyToCreatePipelineLayout() )
            {
                if ( !TryCreateRHIComputePipelineStateForEntry( computeEntry, pDevice ) )
                {
                    m_retryComputePipelineCaches.push_back( computeEntry );
                    bHasFailure = true;
                }
            }
        }

        if ( !m_waitToRegisteredComputePipelines.empty() )
        {
            m_waitToRegisteredComputePipelines.clear();
            std::swap( m_waitToRegisteredComputePipelines, m_retryComputePipelineCaches );
        }

        return !bHasFailure;
    }

	void PipelineRegistry::UnloadAllPipelineShaders()
	{
        if ( m_pResourceSystem )
        {
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

            EE_ASSERT( m_computePipelineStatesCache.size() == m_computePipelineHandlesCache.size() );
            for ( auto const& pPipeline : m_computePipelineStatesCache )
            {
                if ( pPipeline->m_computeShader.IsLoaded() )
                {
                    m_pResourceSystem->UnloadResource( pPipeline->m_computeShader, Resource::ResourceRequesterID( pPipeline->GetID().m_ID ) );
                }
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
        auto* pPipelineState = pDevice->CreateRasterPipeline( rasterEntry->m_desc, compiledShaders );
        if ( pPipelineState )
        {
            rasterEntry->m_pPipelineState = pPipelineState;

            // TODO: add render graph debug name
            EE_LOG_INFO( "Render", "PipelineRegistry", "[%u] Pipeline Visible.", rasterEntry->m_desc.GetHash() );

            return true;
        }

        return false;
    }

    bool PipelineRegistry::TryCreateRHIComputePipelineStateForEntry( TSharedPtr<ComputePipelineEntry>& computeEntry, RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( !computeEntry->IsVisible() );
        EE_ASSERT( Threading::IsMainThread() );

        // Safety: We make sure compute pipeline state layout will only be created by single thread,
        //         and it ResourcePtr is loaded and will not be changed by RHIDevice.
        auto* pPipelineState = pDevice->CreateComputePipeline( computeEntry->m_desc, computeEntry->m_computeShader.GetPtr() );
        if ( pPipelineState )
        {
            computeEntry->m_pPipelineState = pPipelineState;

            // TODO: add render graph debug name
            EE_LOG_INFO( "Render", "PipelineRegistry", "[%u] Pipeline Visible.", computeEntry->m_desc.GetHash() );

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
                    auto const& computeEntry = m_computePipelineStatesCache.Get( pipelineHandle );
                    return ( *computeEntry )->IsVisible();
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

    RHI::RHIPipelineState* PipelineRegistry::TryGetRHIPipelineHandle( PipelineHandle const& pipelineHandle ) const
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
                    auto& computeEntry = *m_computePipelineStatesCache.Get( pipelineHandle );
                    if ( computeEntry->IsVisible() )
                    {
                        return computeEntry->m_pPipelineState;
                    }

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

