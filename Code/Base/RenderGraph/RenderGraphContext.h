#pragma once

#include "RenderGraphNodeRef.h"
#include "RenderGraphResource.h"
#include "Base/Math/Math.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Render/RenderShader.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Optional.h"
#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHITexture.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHICommandBuffer;
    class RHIRenderPass;
    class RHIPipelineState;

    class RHISemaphore;
}

namespace EE::RG
{
    struct RGRenderTargetViewDesc
    {
        RGNodeResourceRef<RGResourceTagTexture, RGResourceViewType::RT>     m_rgRenderTargetRef;
        RHI::RHITextureViewCreateDesc                                       m_viewDesc;
    };

    class EE_BASE_API RGBoundPipeline
    {
        friend class RGRenderCommandContext;

    public:

        void Bind( uint32_t set, TSpan<RGPipelineBinding const> bindings );

        inline void UpdateRHIBinding( uint32_t set, uint32_t binding, RHI::RHIPipelineBinding const& rhiPipelineBinding );

    private:

        RGBoundPipeline( RHI::RHIDeviceRef& pDevice, RHI::RHICommandBuffer* pCommandBuffer, RHI::RHIPipelineState const* pPipelineState, RenderGraph const* pRenderGraph );

        // Helper functions
        //-------------------------------------------------------------------------

        RHI::RHIPipelineBinding ToRHIPipelineBinding( RGPipelineBinding const& pipelineBinding );

        // Variant helper functions
        template <typename BindingType>
        inline auto IsRGBinding( RGPipelineBinding const& pipelineBinding )
        {
            return pipelineBinding.m_binding.index() == GetVariantTypeIndex<decltype( pipelineBinding.m_binding ), BindingType>();
        }

    private:

        using RGPipelineSetBinding = TPair<uint32_t, TSpan<RGPipelineBinding const>>;

        RHI::RHIDeviceRef                                   m_pDevice;
        RHI::RHICommandBuffer*                              m_pCommandBuffer = nullptr;
        RHI::RHIPipelineState const*                        m_pPipelineState = nullptr;

        RenderGraph const*                                  m_pRenderGraph = nullptr;
    };

    class RGExecutableNode;

    class EE_BASE_API RGRenderCommandContext final
    {
        friend class RenderGraph;

    public:

        inline RHI::RHICommandBufferRef const& GetRHICommandBuffer() const { return m_pCommandBuffer; }
        inline RHI::RHIDeviceRef const& GetRHIDevice() const { return m_pDevice; }

        // Concrete Resources
        //-------------------------------------------------------------------------

        template <RGResourceViewType View>
        RHI::RHIBuffer* GetCompiledBufferResource( RGNodeResourceRef<RGResourceTagBuffer, View> const& resourceRef )
        {
            EE_ASSERT( m_pRenderGraph );
            return m_pRenderGraph->GetResourceRegistry().GetCompiledBufferResource( resourceRef );
        }

        template <RGResourceViewType View>
        RHI::RHITexture* GetCompiledTextureResource( RGNodeResourceRef<RGResourceTagTexture, View> const& resourceRef )
        {
            EE_ASSERT( m_pRenderGraph );
            return m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( resourceRef );
        }

        template <RGResourceViewType View>
        RHI::RHIBufferCreateDesc const& GetDesc( RGNodeResourceRef<RGResourceTagBuffer, View> const& resourceRef ) const
        {
            EE_ASSERT( m_pRenderGraph );
            return m_pRenderGraph->GetResourceRegistry().GetCompiledBufferResource( resourceRef )->GetDesc();
        }

        template <RGResourceViewType View>
        RHI::RHITextureCreateDesc const& GetDesc( RGNodeResourceRef<RGResourceTagTexture, View> const& resourceRef ) const
        {
            EE_ASSERT( m_pRenderGraph );
            return m_pRenderGraph->GetResourceRegistry().GetCompiledTextureResource( resourceRef )->GetDesc();
        }

        // Render Commands
        //-------------------------------------------------------------------------

        bool BeginRenderPass(
            RHI::RHIRenderPass* pRenderPass, Int2 extent,
            TSpan<RGRenderTargetViewDesc> colorAttachemnts,
            TOptional<RGRenderTargetViewDesc> depthAttachment = {}
        );
        bool BeginRenderPassWithClearValue(
            RHI::RHIRenderPass* pRenderPass, Int2 extent,
            RHI::RenderPassClearValue const& clearValue,
            TSpan<RGRenderTargetViewDesc> colorAttachemnts,
            TOptional<RGRenderTargetViewDesc> depthAttachment = {}
        );
        inline void EndRenderPass() { EE_ASSERT( m_pCommandBuffer ); m_pCommandBuffer->EndRenderPass(); }

        RGBoundPipeline BindPipeline();

        inline void BindVertexBuffer( uint32_t firstBinding, TSpan<RHI::RHIBuffer const*> pVertexBuffers, uint32_t offset = 0 );
        inline void BindIndexBuffer( RHI::RHIBuffer const* pIndexBuffer, uint32_t offset = 0 );

        inline void Draw( uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0 )
        {
            EE_ASSERT( m_pCommandBuffer );
            m_pCommandBuffer->Draw(vertexCount, instanceCount, firstIndex, firstInstance);
        }
        inline void DrawIndexed( uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0 )
        {
            EE_ASSERT( m_pCommandBuffer );
            m_pCommandBuffer->DrawIndexed( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
        }

        void SetViewport( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 );
        void SetScissor( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 );

        void SetViewportAndScissor( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 );

        // Compute Commands
        //-------------------------------------------------------------------------

        void Dispatch();

        // High level utility interfaces
        //-------------------------------------------------------------------------
        
        // Note: default depth value is 0.0f, reverse z
        void ClearDepthStencil( RGNodeResourceRef<RGResourceTagTexture, RGResourceViewType::UAV> const& depthStencilTexture, float depthValue = 0.0f, uint32_t stencilValue = 0 );

    private:

        // Submit commands to its command queue and reset all context.
        void SubmitAndReset( RHI::RHIDevice* pDevice );

        void AddWaitSyncPoint( RHI::RHISemaphore* pWaitSemaphore, Render::PipelineStage waitStage );
        void AddSignalSyncPoint( RHI::RHISemaphore* pSignalSemaphore );

        template <RGResourceViewType View>
        inline RHI::RenderResourceBarrierState GetResourceCurrentBarrierState( RGNodeResourceRef<RGResourceTagTexture, View> const& resourceRef ) const
        {
            EE_ASSERT( m_pRenderGraph );
            return m_pRenderGraph->GetResourceRegistry().GetCompiledResourceBarrierState( resourceRef );
        }

    private:

        // functions only accessible by RenderGraph.
        void SetCommandContext( RenderGraph const* pRenderGraph, RHI::RHIDeviceRef& pDevice, RHI::RHICommandBuffer* pCommandBuffer );
        inline bool IsValid() const { return m_pRenderGraph && m_pDevice && m_pCommandBuffer; }
        void Reset();

    private:

        RenderGraph const*                  m_pRenderGraph = nullptr;
        RGExecutableNode const*             m_pExecutingNode = nullptr;

        RHI::RHIDeviceRef                   m_pDevice;
        RHI::RHICommandQueueRef             m_pCommandQueue;
        RHI::RHICommandBufferRef            m_pCommandBuffer;

        TVector<RHI::RHISemaphore*>         m_waitSemaphores;
        TVector<Render::PipelineStage>      m_waitStages;
        TVector<RHI::RHISemaphore*>         m_signalSemaphores;
    };
}
