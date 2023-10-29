#pragma once
#include "RenderTarget.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::RHI
{
    class RHISwapchain;
}

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
            EE_ASSERT( m_pSwapChain == nullptr );
            //EE_ASSERT( m_pRhiSwapchain == nullptr );
            EE_ASSERT( !m_renderTarget.IsValid() );
        }

        RenderTarget const* GetRenderTarget() const { return &m_renderTarget; }
        inline bool IsValid() const { return m_pSwapChain != nullptr; }

    protected:

        void*                       m_pSwapChain = nullptr;
        RHI::RHISwapchain*          m_pRhiSwapchain = nullptr;
        RenderTarget                m_renderTarget;
    
        TVector<RenderTarget>       m_RhiRenderTargets;
    };
}