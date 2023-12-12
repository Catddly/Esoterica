#include "RenderWindow.h"

namespace EE::Render
{
    void RenderWindow::AcquireRenderTarget()
    {
        EE_ASSERT( m_renderTarget.AcquireNextFrame() );
    }
}