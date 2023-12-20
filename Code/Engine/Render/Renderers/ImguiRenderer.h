#pragma once

#include "Engine/_Module/API.h"
#include "imgui.h"
#include "Engine/Render/IRenderer.h"
#include "Base/Render/RenderDevice.h"

//-------------------------------------------------------------------------

namespace EE::RHI
{
    class RHIBuffer;
    class RHIRenderPass;
}

namespace EE::RG
{
    class RenderGraph;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    class Viewport;

    //-------------------------------------------------------------------------\

    class EE_ENGINE_API ImguiRenderer final : public IRenderer
    {
    public:

        EE_RENDERER_ID( ImguiRenderer, RendererPriorityLevel::DevelopmentTools );

    private:

        struct RecordedCmdBuffer
        {
            TVector<ImDrawCmd>                  m_cmdBuffer;
            uint32_t                              m_numVertices;
        };

    public:

        bool IsInitialized() const { return m_initialized; }
        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown();

        virtual void RenderViewport_Test( RG::RenderGraph& renderGraph, Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget ) override final;

    private:

        void RenderImguiData( RG::RenderGraph& renderGraph, RenderTarget const& renderTarget, ImDrawData const* pDrawData );

    private:
        
        RenderDevice*                       m_pRenderDevice = nullptr;

        //VertexShader                    m_vertexShader;
        //PixelShader                     m_pixelShader;
        //ShaderInputBindingHandle        m_inputBinding;
        //BlendState                      m_blendState;
        //SamplerState                    m_samplerState;
        //RasterizerState                 m_rasterizerState;
        //TVector<ScissorRect>            m_scissorRects;
        //RenderBuffer                    m_indexBuffer;
        //VertexBuffer                    m_vertexBuffer;
        //Texture                         m_fontTexture;

        //RHI::RHIBuffer*                     m_pVertexBuffer = nullptr;
        //RHI::RHIBuffer*                     m_pIndexBuffer = nullptr;

        RHI::RHIRenderPass*                 m_pRenderPass = nullptr;

        // TODO: we change the origin PipelineState to RasterPipelineState
        //RasterPipelineState             m_PSO;
        bool                                m_initialized = false;
    };
}
#endif