#pragma once

#include "RHITaggedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Memory/Pointers.h"
#include "Base/Math/Math.h"
#include "Base/Render/RenderTarget.h"
#include "Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHITexture;
    class RHISemaphore;

    struct SwapchainTexture
    {
        RHITexture*                 m_pRenderTarget;
        RHISemaphore*               m_pTextureAcquireSemaphore;
        RHISemaphore*               m_pRenderCompleteSemaphore;
        uint32_t                    m_frameIndex;
    };

    class RHISwapchain : public RHITaggedType
    {
    public:

        RHISwapchain( ERHIType rhiType )
            : RHITaggedType( rhiType )
        {}
        virtual ~RHISwapchain() = default;

        RHISwapchain( RHISwapchain const& ) = delete;
        RHISwapchain& operator=( RHISwapchain const& ) = delete;

        RHISwapchain( RHISwapchain&& ) = default;
        RHISwapchain& operator=( RHISwapchain&& ) = default;

    public:

        virtual bool Resize( Int2 const& dimensions ) = 0;

        // Acquire next frame render target.
        // If the texture of next frame render target is still rendering or presenting,
        // this function will block until it is available.
        virtual SwapchainTexture AcquireNextFrameRenderTarget() = 0;

        // Present swapchain render target.
        virtual void Present( SwapchainTexture&& swapchainRenderTarget ) = 0;

        virtual RHI::RHITextureCreateDesc GetPresentTextureDesc() const = 0;
        virtual TVector<RHI::RHITexture*> const GetPresentTextures() const = 0;
    };

    //-------------------------------------------------------------------------

    using RHISwapchainRef = TTSSharedPtr<RHISwapchain>;
}