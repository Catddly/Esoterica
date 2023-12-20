#include "RenderWindow.h"
#include "Base/RHI/RHISwapchain.h"

namespace EE::Render
{
    void RenderWindow::AcquireRenderTarget()
    {
        EE_ASSERT( m_renderTarget.AcquireNextFrame() );
    }

	void RenderWindow::Present()
	{
        if ( m_renderTarget.IsInitialized() )
        {
            auto* pSwapchain = m_renderTarget.GetRHISwapchain();
            if ( pSwapchain )
            {
                pSwapchain->Present( RHI::SwapchainTexture {
                    m_renderTarget.m_pRenderTarget,
                    m_renderTarget.m_pTextureAcquireSemaphore,
                    m_renderTarget.m_pRenderCompleteSemaphore,
                    m_renderTarget.m_frameIndex
                } );
            }

            m_renderTarget.ResetFrame();
        }
    }
}