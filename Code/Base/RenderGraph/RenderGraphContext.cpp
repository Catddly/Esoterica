#include "RenderGraphContext.h"
#include "RenderGraph.h"
#include "Base/Types/Map.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIPipelineState.h"
#include "Base/RHI/Resource/RHITexture.h"
#include "Base/RHI/Resource/RHIRenderPass.h"
#include "Base/RHI/RHICommandBuffer.h"

namespace EE::RG
{
    void RGBoundPipeline::Bind( uint32_t set, TSpan<RGPipelineBinding const> bindings )
    {
        EE_ASSERT( set < RHI::NumMaxResourceBindingSet );
        EE_ASSERT( m_pDevice );
        EE_ASSERT( m_pCommandBuffer );
        EE_ASSERT( m_pPipelineState );
        EE_ASSERT( m_pRenderGraph );

        auto const& resourceLayouts = m_pPipelineState->GetResourceSetLayouts();

        if ( set >= resourceLayouts.size() )
        {
            EE_LOG_WARNING( "RenderGraph", "Pipeline Set Binding", "Try to bind to a set which doesn't exist!" );
            return;
        }

        if ( resourceLayouts[set].empty() )
        {
            EE_LOG_WARNING( "RenderGraph", "Pipeline Set Binding", "Try to bind to a set which is empty!" );
            return;
        }

        TInlineVector<RHI::RHIPipelineBinding const, 16> rhiBindings;
        rhiBindings.reserve( bindings.size() );

        for ( RGPipelineBinding const& binding : bindings )
        {
            rhiBindings.emplace_back( ToRHIPipelineBinding( binding ) );
        }

        m_pCommandBuffer->BindDescriptorSetInPlace( set, m_pPipelineState, rhiBindings );
    }

    void RGBoundPipeline::UpdateRHIBinding( uint32_t set, uint32_t binding, RHI::RHIPipelineBinding const& rhiPipelineBinding )
    {
        m_pCommandBuffer->UpdateDescriptorSetBinding( set, binding, m_pPipelineState, rhiPipelineBinding );
    }

    RGBoundPipeline::RGBoundPipeline( RHI::RHIDevice* pDevice, RHI::RHICommandBuffer* pCommandBuffer, RHI::RHIPipelineState const* pPipelineState, RenderGraph const* pRenderGraph )
        : m_pDevice( pDevice ), m_pCommandBuffer( pCommandBuffer ), m_pPipelineState( pPipelineState ), m_pRenderGraph( pRenderGraph )
    {
    }

    //-------------------------------------------------------------------------

    RHI::RHIPipelineBinding RGBoundPipeline::ToRHIPipelineBinding( RGPipelineBinding const& pipelineBinding )
    {
        RHI::RHIPipelineBinding rhiBinding = RHI::RHIUnknownBinding{};

        if ( IsRGBinding<RGPipelineBufferBinding>( pipelineBinding ) )
        {
            auto const& bufferBinding = eastl::get<RGPipelineBufferBinding>( pipelineBinding.m_binding );

            RHI::RHIBuffer* pBuffer = m_pRenderGraph->GetResourceRegistry().GetCompiledResource( bufferBinding.m_resourceId ).GetResource<RGResourceTagBuffer>();
            EE_ASSERT( pBuffer );
            rhiBinding = RHI::RHIPipelineBinding( RHI::RHIBufferBinding{ pBuffer } );
        }
        else if ( IsRGBinding<RGPipelineDynamicBufferBinding>( pipelineBinding ) )
        {
            auto const& dynBufferBinding = eastl::get<RGPipelineDynamicBufferBinding>( pipelineBinding.m_binding );

            RHI::RHIBuffer* pBuffer = m_pRenderGraph->GetResourceRegistry().GetCompiledResource( dynBufferBinding.m_resourceId ).GetResource<RGResourceTagBuffer>();
            EE_ASSERT( pBuffer );
            rhiBinding = RHI::RHIPipelineBinding( RHI::RHIDynamicBufferBinding{ pBuffer, dynBufferBinding.m_dynamicOffset } );
        }
        else if ( IsRGBinding<RGPipelineTextureBinding>( pipelineBinding ) )
        {
            auto const& textureBinding = eastl::get<RGPipelineTextureBinding>( pipelineBinding.m_binding );

            RHI::RHITexture* pTexture = m_pRenderGraph->GetResourceRegistry().GetCompiledResource( textureBinding.m_resourceId ).GetResource<RGResourceTagTexture>();
            EE_ASSERT( pTexture );
            rhiBinding = RHI::RHIPipelineBinding{ RHI::RHITextureBinding{ pTexture->GetOrCreateView( m_pDevice, textureBinding.m_viewDesc ), textureBinding.m_layout } };
        }
        else if ( IsRGBinding<RGPipelineTextureArrayBinding>( pipelineBinding ) )
        {
            auto const& textureArrayBinding = eastl::get<RGPipelineTextureArrayBinding>( pipelineBinding.m_binding );

            auto rhiTextureArrayBinding = RHI::RHITextureArrayBinding{};
            rhiTextureArrayBinding.m_bindings.reserve( textureArrayBinding.m_bindings.size() );
            for ( auto const& texBindingElement : textureArrayBinding.m_bindings )
            {
                RHI::RHITexture* pTexture = m_pRenderGraph->GetResourceRegistry().GetCompiledResource( texBindingElement.m_resourceId ).GetResource<RGResourceTagTexture>();
                EE_ASSERT( pTexture );
                rhiTextureArrayBinding.m_bindings.emplace_back( RHI::RHITextureBinding{ pTexture->GetOrCreateView( m_pDevice, texBindingElement.m_viewDesc ), texBindingElement.m_layout } );
            }

            rhiBinding = RHI::RHIPipelineBinding{ eastl::move( rhiTextureArrayBinding ) };
        }
        else if ( IsRGBinding<RGPipelineStaticSamplerBinding>( pipelineBinding ) )
        {
            rhiBinding = RHI::RHIPipelineBinding{ RHI::RHIStaticSamplerBinding{} };
        }
        else if ( IsRGBinding<RGPipelineUnknownBinding>( pipelineBinding ) )
        {
            rhiBinding = RHI::RHIPipelineBinding{ RHI::RHIUnknownBinding{} };
        }
        else if ( IsRGBinding<RGPipelineRHIRawBinding>( pipelineBinding ) )
        {
            auto const& rhiRawBinding = eastl::get<RGPipelineRHIRawBinding>( pipelineBinding.m_binding );
            rhiBinding = rhiRawBinding.m_rhiPipelineBinding;
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }

        return rhiBinding;
    }

    // Render Commands
    //-------------------------------------------------------------------------

    bool RGRenderCommandContext::BeginRenderPass(
        RHI::RHIRenderPass* pRenderPass, Int2 extent,
        TSpan<RGRenderTargetViewDesc> colorAttachemnts,
        TOptional<RGRenderTargetViewDesc> depthAttachment
    )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        EE_ASSERT( extent.m_x > 0 && extent.m_y > 0 );

        // get or create necessary framebuffer
        //-------------------------------------------------------------------------

        RHI::RHIFramebufferCacheKey key;
        key.m_extentX = static_cast<uint32_t>( extent.m_x );
        key.m_extentY = static_cast<uint32_t>( extent.m_y );

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

        TFixedVector<RHI::RHITextureView, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> textureViews;

        for ( RGRenderTargetViewDesc const& color : colorAttachemnts )
        {
            RHI::RHITexture* pRenderTarget = m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( color.m_rgRenderTargetRef );
            RHI::RHITextureView pRenderTargetView = pRenderTarget->GetOrCreateView( m_pDevice, color.m_viewDesc );
            textureViews.push_back( pRenderTargetView );
        }

        if ( depthAttachment.has_value() )
        {
            RHI::RHITexture* pRenderTarget = m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( depthAttachment->m_rgRenderTargetRef );
            RHI::RHITextureView pRenderTargetView = pRenderTarget->GetOrCreateView( m_pDevice, depthAttachment->m_viewDesc );
            textureViews.push_back( pRenderTargetView );
        }

        auto renderArea = RHI::RenderArea{ key.m_extentX, key.m_extentY, 0u, 0u };
        return m_pCommandBuffer->BeginRenderPass( pRenderPass, pFramebuffer, renderArea, textureViews );
    }

    RGBoundPipeline RGRenderCommandContext::BindPipeline()
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        auto* pPipelineState = m_pExecutingNode->m_pPipelineState;
        EE_ASSERT( pPipelineState );

        m_pCommandBuffer->BindPipelineState( pPipelineState );

        return RGBoundPipeline( m_pDevice, m_pCommandBuffer, pPipelineState, m_pRenderGraph );
    }

    void RGRenderCommandContext::BindVertexBuffer( uint32_t firstBinding, TSpan<RHI::RHIBuffer*> pVertexBuffers, uint32_t offset )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        m_pCommandBuffer->BindVertexBuffer( firstBinding, pVertexBuffers, offset );
    }

    void RGRenderCommandContext::BindIndexBuffer( RHI::RHIBuffer* pIndexBuffer, uint32_t offset )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        m_pCommandBuffer->BindIndexBuffer( pIndexBuffer, offset );
    }

    void RGRenderCommandContext::SetViewport( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        m_pCommandBuffer->SetViewport( width, height, xOffset, yOffset );
    }

    void RGRenderCommandContext::SetScissor( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_pExecutingNode );
        m_pCommandBuffer->SetScissor( width, height, xOffset, yOffset );
    }
    
    //-------------------------------------------------------------------------

    void RGRenderCommandContext::SubmitAndReset( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( pDevice );
        pDevice->SubmitCommandBuffer( m_pCommandBuffer, m_waitSemaphores, m_signalSemaphores, m_waitStages );
        Reset();
    }

    void RGRenderCommandContext::AddWaitSyncPoint( RHI::RHISemaphore* pWaitSemaphore, Render::PipelineStage waitStage )
    {
        EE_ASSERT( m_waitSemaphores.size() == m_waitStages.size() );

        m_waitSemaphores.push_back( pWaitSemaphore );
        m_waitStages.push_back( waitStage );
    }

    void RGRenderCommandContext::AddSignalSyncPoint( RHI::RHISemaphore* pSignalSemaphore )
    {
        m_signalSemaphores.push_back( pSignalSemaphore );
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
        m_pCommandQueue = nullptr;
        m_pCommandBuffer = nullptr;

        m_waitSemaphores.clear();
        m_waitStages.clear();
        m_signalSemaphores.clear();
    }
}
