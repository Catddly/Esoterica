#include "RenderGraphResource.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "Base/RHI/Resource/RHITexture.h"

namespace EE::RG
{
    RGCompiledResource RGResource::Compile( RHI::RHIDevice* pDevice ) &&
    {
        RGCompiledResource compiled;

        switch ( GetResourceType() )
        {
            case RGResourceType::Buffer:
            {
                BufferDesc const& desc = GetDesc<RGResourceTagBuffer>();

                EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Buffer Desired Size = %d", desc.m_desc.m_desireSize );
                EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Buffer Allocated Size = %d", desc.m_desc.m_desireSize );

                if ( !IsImportedResource() )
                {
                    auto* pBuffer = pDevice->CreateBuffer( desc.m_desc );
                    EE_ASSERT( pBuffer != nullptr );
                    compiled.m_resource = pBuffer;
                    // Note: because this resource is created by the render graph within this frame, it doesn't matter it first barrier state is.
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };
;
                    EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Lazy created buffer resource:" );
                }
                else
                {
                    compiled.m_importedResource = eastl::exchange( eastl::get<_Impl::ImportedResourceVariantIndex>( m_resource ), {} );
                    // Safety: We make sure this shared pointer will be alive as long as this raw pointer alive.
                    RHI::RHIResource* pResource = compiled.m_importedResource->m_pImportedResource;
                    compiled.m_resource = static_cast<RHI::RHIBuffer*>( pResource );
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ compiled.m_importedResource->m_currentAccess };
                    EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Import a buffer resource:" );
                }
            }
            break;

            case RGResourceType::Texture:
            {
                TextureDesc const& desc = GetDesc<RGResourceTagTexture>();

                EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Texture Width = %d", desc.m_desc.m_width );
                EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Texture Height = %d", desc.m_desc.m_height );

                if ( !IsImportedResource() )
                {
                    auto* pTexture = pDevice->CreateTexture( desc.m_desc );
                    EE_ASSERT( pTexture != nullptr );
                    compiled.m_resource = pTexture;
                    // Note: because this resource is created by the render graph within this frame, it doesn't matter it first barrier state is.
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ RHI::RenderResourceBarrierState::Undefined };

                    EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Lazy created texture resource:" );
                }
                else
                {
                    compiled.m_importedResource = eastl::exchange( eastl::get<_Impl::ImportedResourceVariantIndex>( m_resource ), {} );
                    // Safety: We make sure this shared pointer will be alive as long as this raw pointer alive.
                    RHI::RHIResource* pResource = compiled.m_importedResource->m_pImportedResource;
                    compiled.m_resource = static_cast<RHI::RHITexture*>( pResource );
                    compiled.m_currentAccessState = RHI::RenderResourceAccessState{ compiled.m_importedResource->m_currentAccess };
                    EE_LOG_MESSAGE( "RenderGraph", "RenderGraph::CreateNodeRHIResource()", "Import a texture resource:" );
                }
            }
            break;

            case RGResourceType::Unknown:
            default:
            EE_LOG_ERROR( "Render Graph", "RenderGraph::Compile()", "Unknown type of render graph resource!" );
            EE_ASSERT( false );
            break;
        }

        compiled.m_desc = eastl::exchange( m_desc, {} );
        return compiled;
    }

    //-------------------------------------------------------------------------

    void RGCompiledResource::Retire( RHI::RHIDevice* pDevice )
    {
        if ( !IsImportedResource() )
        {
            switch ( GetResourceType() )
            {
                case RGResourceType::Buffer:
                {
                    auto& pResource = GetResource<RGResourceTagBuffer>();
                    RHI::RHIBuffer* pRhiBuffer = static_cast<RHI::RHIBuffer*>( pResource );
                    pDevice->DestroyBuffer( pRhiBuffer );
                    pResource = nullptr;
                }
                break;

                case RGResourceType::Texture:
                {
                    auto& pResource = GetResource<RGResourceTagTexture>();
                    RHI::RHITexture* pRhiTexture = static_cast<RHI::RHITexture*>( pResource );
                    pDevice->DestroyTexture( pRhiTexture );
                    pResource = nullptr;
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
}