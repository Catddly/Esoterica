#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Event.h"
#include "Base/RHI/Resource/RHIResource.h"
#include "RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::RHI
{
    class RHIDevice;
    class RHISwapchain;

    class RHITexture;
    class RHISemaphore;
}

namespace EE::Render
{
    struct PickingID
    {
        PickingID() = default;
        inline bool IsSet() const { return m_0 != 0 || m_1 != 0; }

        uint64_t m_0 = 0;
        uint64_t m_1 = 0;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API RenderTarget : public RHI::IRHIResourceWrapper
    {
        friend class RenderDevice;

    public:

        struct RenderTargetCreateParameters : public ResourceCreateParameters
        {
            uint32_t                    m_width = 0;
            uint32_t                    m_height = 0;
            bool                        m_bNeedDepth = true;
            bool                        m_bNeedIdPicking = false;
        };

    public:

        RenderTarget() = default;
        RenderTarget( RenderTarget const& ) = default;
        virtual ~RenderTarget() { EE_ASSERT( m_pRenderTarget == nullptr ); }

        inline bool IsValid() const { return m_pRenderTarget != nullptr; }
        inline bool HasDepthStencil() const { return m_pDepthStencil != nullptr; }
        inline bool HasPickingRT() const { return m_pPickingRT != nullptr; }
        inline Int2 GetDimensions() const { return m_dimensions; }

        inline bool operator==( RenderTarget const& rhs ) const { return m_pRenderTarget == rhs.m_pRenderTarget && m_pDepthStencil == rhs.m_pDepthStencil; }
        inline bool operator!=( RenderTarget const& rhs ) const { return !operator==(rhs); }

        inline RHI::RHITexture const* GetRHIRenderTarget() const { return m_pRenderTarget; }
        inline RHI::RHITexture const* GetRHIDepthStencil() const { return m_pDepthStencil; }
        inline RHI::RHITexture const* GetRHIPickingRenderTarget() const { return m_pPickingRT; }

        inline RHI::RHITexture* GetRHIRenderTarget() { return m_pRenderTarget; }
        inline RHI::RHITexture* GetRHIDepthStencil() { return m_pDepthStencil; }
        inline RHI::RHITexture* GetRHIPickingRenderTarget() { return m_pPickingRT; }

    public:

        virtual bool IsSwapchainRenderTarget() const { return false; }

        virtual void Release( RHI::RHIDevice* pDevice ) override;

    public:

        bool Initialize( RHI::RHIDevice* pDevice, RenderTargetCreateParameters const& createParams ) { return InitializeBase( pDevice, createParams ); }

    protected:

        virtual bool InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams ) override;

    protected:

        Int2                        m_dimensions;

        RHI::RHITexture*            m_pRenderTarget = nullptr;
        RHI::RHITexture*            m_pDepthStencil = nullptr;
        RHI::RHITexture*            m_pPickingRT = nullptr;
        RHI::RHITexture*            m_pPickingStagingRT = nullptr;

        Texture                     m_RT;
        Texture                     m_DS;
        Texture                     m_pickingRT;
        Texture                     m_pickingStagingTexture;
    };

    // Render target which belongs to a RenderWindow.
    class SwapchainRenderTarget final : public RenderTarget
    {
        friend class RenderWindow;
        friend class RenderGraph;

    public:

        struct SwapchainRenderTargetCreateParameters : public ResourceCreateParameters
        {
            RHI::RHISwapchain*              m_pSwapchain = nullptr;
        };

    public:

        virtual bool IsSwapchainRenderTarget() const override { return true; }

        virtual void Release( RHI::RHIDevice* pDevice ) override;

    public:

        bool Initialize( RHI::RHIDevice* pDevice, SwapchainRenderTargetCreateParameters const& createParams ) { return InitializeBase( pDevice, createParams ); }

        void ResetFrame();

        bool AcquireNextFrame();

        RHI::RHISwapchain const* GetRHISwapchain() const { return m_pSwapchain; }
        RHI::RHISemaphore const* GetWaitSemaphore() const { return m_pTextureAcquireSemaphore; }
        RHI::RHISemaphore const* GetSignalSemaphore() const { return m_pRenderCompleteSemaphore; }

        RHI::RHISwapchain* GetRHISwapchain() { return m_pSwapchain; }
        RHI::RHISemaphore* GetWaitSemaphore() { return m_pTextureAcquireSemaphore; }
        RHI::RHISemaphore* GetSignalSemaphore() { return m_pRenderCompleteSemaphore; }

    private:

        virtual bool InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams ) override;

    private:

        RHI::RHISwapchain*              m_pSwapchain = nullptr;

        RHI::RHISemaphore*              m_pTextureAcquireSemaphore = nullptr;
        RHI::RHISemaphore*              m_pRenderCompleteSemaphore = nullptr;
        uint32_t                        m_frameIndex = 0;

        EventBindingID                  m_onSwapchainTextureDestroyedEventId;
    };
}