#pragma once

#include "Base/_Module/API.h"
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

    class RenderTarget : public RHI::IRHIResourceWrapper
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
        virtual ~RenderTarget() { EE_ASSERT( m_pRenderTarget != nullptr ); }

        inline bool IsValid() const { return m_pRenderTarget != nullptr; }
        Int2 GetDimensions() const;
        inline bool HasDepthStencil() const { return m_pDepthStencil != nullptr; }
        inline bool HasPickingRT() const { return m_pPickingRT != nullptr; }

        inline bool operator==( RenderTarget const& rhs ) const { return m_pRenderTarget == rhs.m_pRenderTarget && m_pDepthStencil == rhs.m_pDepthStencil; }
        inline bool operator!=( RenderTarget const& rhs ) const { return !operator==(rhs); }

        inline RHI::RHITexture* GetRHIRenderTarget() const { return m_pRenderTarget; }

        //inline ViewSRVHandle const& GetRenderTargetShaderResourceView() const { EE_ASSERT( m_RT.IsValid() ); return m_RT.GetShaderResourceView(); }
        //inline ViewRTHandle const& GetRenderTargetHandle() const { EE_ASSERT( m_RT.IsValid() ); return m_RT.GetRenderTargetView(); }
        //inline ViewRTHandle const& GetPickingRenderTargetHandle() const { EE_ASSERT( m_pickingRT.IsValid() ); return m_pickingRT.GetRenderTargetView(); }
        //inline ViewDSHandle const& GetDepthStencilHandle() const { EE_ASSERT( m_DS.IsValid() ); return m_DS.GetDepthStencilView(); }

    public:

        virtual bool IsSwapchainRenderTarget() const { return false; }

    public:

        bool Initialize( RHI::RHIDevice* pDevice, RenderTargetCreateParameters const& createParams ) { return InitializeBase( pDevice, createParams ); }

        virtual void Release( RHI::RHIDevice* pDevice ) override;

    protected:

        virtual bool InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams ) override;

    protected:

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

    public:

        struct SwapchainRenderTargetCreateParameters : public ResourceCreateParameters
        {
            RHI::RHISwapchain*              m_pSwapchain = nullptr;
        };

    public:

        bool Initialize( RHI::RHIDevice * pDevice, SwapchainRenderTargetCreateParameters const& createParams ) { return InitializeBase( pDevice, createParams ); }

        virtual void Release( RHI::RHIDevice* pDevice ) override;

        bool AcquireNextFrame();

        RHI::RHISwapchain const* GetRHISwapchain() const { return m_pSwapchain; }

    public:

        virtual bool IsSwapchainRenderTarget() const override { return true; }

    private:

        virtual bool InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams ) override;

    private:

        RHI::RHISwapchain*              m_pSwapchain = nullptr;

        RHI::RHISemaphore*              m_pTextureAcquireSemaphore = nullptr;
        RHI::RHISemaphore*              m_pRenderCompleteSemaphore = nullptr;
        uint32_t                        m_frameIndex = 0;
    };
}