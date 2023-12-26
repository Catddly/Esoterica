#include "RenderGraphResource.h"
#include "RenderGraphResourceRegistry.h"
#include "RenderGraphTransientResourceCache.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "Base/RHI/Resource/RHITexture.h"

namespace EE::RG
{
    RGCompiledResource RGResource::Compile( RHI::RHIDevice* pDevice, RGResourceRegistry& registry, RGTransientResourceCache& cache ) &&
    {
        RGCompiledResource compiled;

        switch ( GetResourceType() )
        {
            case RGResourceType::Buffer:
            {
                BufferDesc const& desc = GetDesc<RGResourceTagBuffer>();

                if ( IsNamedResource() )
                {
                    cache.UpdateDirtyNamedBuffer( m_name, pDevice, desc.m_desc );
                    auto* pBuffer = cache.GetOrCreateNamedBuffer( m_name, pDevice, desc.m_desc );

                    EE_ASSERT( pBuffer != nullptr );

                    // use last frame's resource barrier state if available
                    auto lastFrameBarrierStateIterator = registry.m_exportedResourceBarrierStates.find( m_name );
                    if ( lastFrameBarrierStateIterator != registry.m_exportedResourceBarrierStates.end() )
                    {
                        compiled.m_currentAccessState = lastFrameBarrierStateIterator->second;
                    }
                    else
                    {
                        compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };
                    }

                    compiled.m_resource = pBuffer;
                    compiled.m_bIsNamedResource = true;
                }
                else if ( !IsImportedResource() )
                {
                    auto* pBuffer = cache.FetchAvailableTemporaryBuffer( desc.m_desc );
                    if ( !pBuffer )
                    {
                        pBuffer = pDevice->CreateBuffer( desc.m_desc );
                    }

                    EE_ASSERT( pBuffer != nullptr );
                    compiled.m_resource = pBuffer;
                    // Note: because this resource is created by the render graph within this frame, it doesn't matter it first barrier state is.
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };
                }
                else
                {
                    compiled.m_importedResource = eastl::exchange( eastl::get<_Impl::ImportedResourceVariantIndex>( m_resource ), {} );
                    // Safety: We make sure this shared pointer will be alive as long as this raw pointer alive.
                    RHI::RHIResource* pResource = compiled.m_importedResource->m_pImportedResource;
                    compiled.m_resource = static_cast<RHI::RHIBuffer*>( pResource );
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ compiled.m_importedResource->m_currentAccess };
                }
            }
            break;

            case RGResourceType::Texture:
            {
                TextureDesc const& desc = GetDesc<RGResourceTagTexture>();

                if ( IsNamedResource() )
                {
                    cache.UpdateDirtyNamedTexture( m_name, pDevice, desc.m_desc );
                    auto* pTexture = cache.GetOrCreateNamedTexture( m_name, pDevice, desc.m_desc );

                    EE_ASSERT( pTexture != nullptr );
                    auto lastFrameBarrierStateIterator = registry.m_exportedResourceBarrierStates.find( m_name );
                    if ( lastFrameBarrierStateIterator != registry.m_exportedResourceBarrierStates.end() )
                    {
                        compiled.m_currentAccessState = lastFrameBarrierStateIterator->second;
                    }
                    else
                    {
                        compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };
                    }
                    compiled.m_resource = pTexture;
                    compiled.m_bIsNamedResource = true;
                }
                else if ( !IsImportedResource() )
                {
                    auto* pTexture = cache.FetchAvailableTemporaryTexture( desc.m_desc );
                    if ( !pTexture )
                    {
                        pTexture = pDevice->CreateTexture( desc.m_desc );
                    }

                    EE_ASSERT( pTexture != nullptr );
                    compiled.m_resource = pTexture;
                    // Note: because this resource is created by the render graph within this frame, it doesn't matter it first barrier state is.
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };
                }
                else
                {
                    compiled.m_importedResource = eastl::exchange( eastl::get<_Impl::ImportedResourceVariantIndex>( m_resource ), {} );
                    // Safety: We make sure this shared pointer will be alive as long as this raw pointer alive.
                    RHI::RHIResource* pResource = compiled.m_importedResource->m_pImportedResource;
                    compiled.m_resource = static_cast<RHI::RHITexture*>( pResource );
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ compiled.m_importedResource->m_currentAccess };
                }
            }
            break;

            case RGResourceType::Unknown:
            default:
            EE_LOG_ERROR( "Render Graph", "RenderGraph::Compile()", "Unknown type of render graph resource!" );
            EE_ASSERT( false );
            break;
        }

        compiled.m_name = eastl::move( m_name );
        compiled.m_desc = eastl::exchange( m_desc, {} );
        compiled.m_bIsNamedResource = m_bIsNamedResource;
        return compiled;
    }

    //-------------------------------------------------------------------------

    void RGCompiledResource::Retire( RGResourceRegistry& resourceRegistry, RGTransientResourceCache& cache )
    {
        if ( IsNamedResource() )
        {
            switch ( GetResourceType() )
            {
                case RGResourceType::Buffer:
                {
                    // remember access state of this exportable resource after this frame is ended
                    resourceRegistry.m_exportedResourceBarrierStates.insert_or_assign( m_name, m_currentAccessState );
                }
                break;

                case RGResourceType::Texture:
                {
                    // remember access state of this exportable resource after this frame is ended
                    resourceRegistry.m_exportedResourceBarrierStates.insert_or_assign( m_name, m_currentAccessState );
                }
                break;

                case RGResourceType::Unknown:
                default:
                EE_LOG_ERROR( "Render Graph", "Compilation", "Unknown type of render graph resource!" );
                EE_ASSERT( false );
                break;
            }
        }
        else if ( !IsImportedResource() )
        {
            switch ( GetResourceType() )
            {
                case RGResourceType::Buffer:
                {
                    auto& pResource = GetResource<RGResourceTagBuffer>();
                    RHI::RHIBuffer* pRhiBuffer = static_cast<RHI::RHIBuffer*>( pResource );
                    pResource = nullptr;

                    cache.RestoreBuffer( pRhiBuffer );
                }
                break;

                case RGResourceType::Texture:
                {
                    auto& pResource = GetResource<RGResourceTagTexture>();
                    RHI::RHITexture* pRhiTexture = static_cast<RHI::RHITexture*>( pResource );
                    pResource = nullptr;

                    cache.RestoreTexture( pRhiTexture );
                }
                break;

                case RGResourceType::Unknown:
                default:
                EE_LOG_ERROR( "Render Graph", "Compilation", "Unknown type of render graph resource!" );
                EE_ASSERT( false );
                break;
            }
        }
    }

    //-------------------------------------------------------------------------

    RGPipelineBinding::RGPipelineBinding( RGPipelineResourceBinding const& bindings )
        : m_binding( bindings )
    {

    }
}