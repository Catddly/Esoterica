#pragma once

#include "RenderGraphNodeRef.h"
#include "RenderGraphResource.h"
#include "Base/Math/Math.h"
#include "Base/Render/RenderShader.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Optional.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIDevice;
    class RHICommandQueue;
    class RHICommandBuffer;
    class RHIRenderPass;
    class RHIPipelineState;
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

        RGBoundPipeline( RHI::RHIDevice* pDevice, RHI::RHICommandBuffer* pCommandBuffer, RHI::RHIPipelineState const* pPipelineState, RenderGraph const* pRenderGraph );

        // Helper functions
        //-------------------------------------------------------------------------

        RHI::RHIPipelineBinding ToRHIPipelineBinding( RGPipelineBinding const& pipelineBinding );

    private:

        using RGPipelineSetBinding = TPair<uint32_t, TSpan<RGPipelineBinding const>>;

        RHI::RHIDevice*                                     m_pDevice = nullptr;
        RHI::RHICommandBuffer*                              m_pCommandBuffer = nullptr;
        RHI::RHIPipelineState const*                        m_pPipelineState = nullptr;

        RenderGraph const*                                  m_pRenderGraph = nullptr;
    };

    class RGExecutableNode;

    class EE_BASE_API RGRenderCommandContext
    {
        friend class RenderGraph;

    public:

        inline RHI::RHICommandBuffer* const& GetRHICommandBuffer() const { return m_pCommandBuffer; }
        inline RHI::RHIDevice* const& GetRHIDevice() const { return m_pDevice; }

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

        // Render Commands
        //-------------------------------------------------------------------------

        bool BeginRenderPass(
            RHI::RHIRenderPass* pRenderPass, Int2 extent,
            TSpan<RGRenderTargetViewDesc> colorAttachemnts,
            TOptional<RGRenderTargetViewDesc> depthAttachment = {}
        );
        inline void EndRenderPass() { EE_ASSERT( m_pCommandBuffer ); m_pCommandBuffer->EndRenderPass(); }

        RGBoundPipeline BindPipeline();

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

    private:

        // functions only accessible by RenderGraph.
        void SetCommandContext( RenderGraph const* pRenderGraph, RHI::RHIDevice* pDevice, RHI::RHICommandBuffer* pCommandBuffer );
        inline bool IsValid() const { return m_pRenderGraph && m_pDevice && m_pCommandBuffer; }
        void Reset();

    private:

        RenderGraph const*                  m_pRenderGraph = nullptr;
        RGExecutableNode const*             m_pExecutingNode = nullptr;

        RHI::RHIDevice*                     m_pDevice = nullptr;
        RHI::RHICommandQueue*               m_pCommandQueue = nullptr;
        RHI::RHICommandBuffer*              m_pCommandBuffer = nullptr;
    };
}
