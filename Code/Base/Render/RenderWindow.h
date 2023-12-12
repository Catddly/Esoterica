#pragma once
#include "RenderTarget.h"
#include "Base/Types/Arrays.h"
#include "Base/RHI/RHISwapchain.h"

namespace EE::Render
{
    class EE_BASE_API RenderWindow
    {
        friend class RenderDevice;
        friend class RenderContext;

    public:

        RenderWindow() = default;
        ~RenderWindow() 
        { 
            EE_ASSERT( m_pSwapchain == nullptr );
            EE_ASSERT( !m_renderTarget.IsValid() );
        }

        Render::RenderTarget const* GetRenderTarget() const { return &m_renderTarget; }
        inline bool IsValid() const { return m_pSwapchain != nullptr; }

        // Acquire render target handles for this RenderWindow.
        // Render target only become valid after this function call.
        void AcquireRenderTarget();

    protected:

        RHI::RHISwapchain*                  m_pSwapchain = nullptr;
        Render::SwapchainRenderTarget       m_renderTarget;
    };
}