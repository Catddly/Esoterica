#include "RenderGraphContext.h"
#include "RenderGraph.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHITexture.h"
#include "Base/RHI/Resource/RHIRenderPass.h"
#include "Base/RHI/RHICommandBuffer.h"

namespace EE
{
    namespace RG
    {
        bool RGRenderCommandContext::BeginRenderPass(
            RHI::RHIRenderPass* pRenderPass, Int2 extent,
            TSpan<RGRenderTargetViewDesc> colorAttachemnts,
            TOptional<RGRenderTargetViewDesc> depthAttachment
        )
        {
            EE_ASSERT( IsValid() );
            EE_ASSERT( extent.m_x > 0 && extent.m_y > 0 );

            // get or create necessary framebuffer
            //-------------------------------------------------------------------------

            RHI::RHIFramebufferCacheKey key;
            key.m_extentX = static_cast<uint32_t>( extent.m_x );
            key.m_extentX = static_cast<uint32_t>( extent.m_y );

            for ( RGRenderTargetViewDesc const& color : colorAttachemnts )
            {
                auto const& desc = color.m_rgRenderTargetRef.GetDesc().m_desc;
                key.m_attachmentHashs.emplace_back( desc.m_usage, desc.m_flag );
            }

            if ( depthAttachment.has_value() )
            {
                auto const& desc = depthAttachment->m_rgRenderTargetRef.GetDesc().m_desc;
                key.m_attachmentHashs.emplace_back( desc.m_usage, desc.m_flag );
            }

            auto* pFramebuffer = pRenderPass->GetOrCreateFramebuffer( m_pDevice, key );
            if ( !pFramebuffer )
            {
                EE_LOG_WARNING( "RenderGraph", "RenderGraphCommandContext", "Failed to fetch framebuffer!" );
                return false;
            }

            // collect texture views
            //-------------------------------------------------------------------------

            TFixedVector<RHI::RHITextureView*, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> textureViews;

            for ( RGRenderTargetViewDesc const& color : colorAttachemnts )
            {
                RHI::RHITexture* pRenderTarget = m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( color.m_rgRenderTargetRef );
                RHI::RHITextureView* pRenderTargetView = pRenderTarget->GetOrCreateView( m_pDevice, color.m_viewDesc );
                textureViews.push_back( pRenderTargetView );
            }

            if ( depthAttachment.has_value() )
            {
                RHI::RHITexture* pRenderTarget = m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( depthAttachment->m_rgRenderTargetRef );
                RHI::RHITextureView* pRenderTargetView = pRenderTarget->GetOrCreateView( m_pDevice, depthAttachment->m_viewDesc );
                textureViews.push_back( pRenderTargetView );
            }

            auto renderArea = RHI::RenderArea{ key.m_extentX, key.m_extentY, 0u, 0u };
            return m_pCommandBuffer->BeginRenderPass( pRenderPass, pFramebuffer, renderArea, textureViews );
        }

        void RGRenderCommandContext::SetViewport( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
        {
            EE_ASSERT( IsValid() );
            m_pCommandBuffer->SetViewport( width, height, xOffset, yOffset );
        }

        void RGRenderCommandContext::SetScissor( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
        {
            EE_ASSERT( IsValid() );
            m_pCommandBuffer->SetScissor( width, height, xOffset, yOffset );
        }
    
        //-------------------------------------------------------------------------

        void RGRenderCommandContext::SetCommandContext( RenderGraph const* pRenderGraph, RHI::RHIDevice* pDevice, RHI::RHICommandBuffer* pCommandBuffer )
        {
            m_pDevice = pDevice;
            m_pCommandQueue = pDevice->GetMainGraphicCommandQueue();
            m_pRenderGraph = pRenderGraph;
            m_pCommandBuffer = pCommandBuffer;
        }

        void RGRenderCommandContext::Reset()
        {
            m_pRenderGraph = nullptr;
            m_pDevice = nullptr;
            m_pCommandBuffer = nullptr;
        }
    }
}
