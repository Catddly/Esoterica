#include "RenderTarget.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/RHISwapchain.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"
#include "Base/RHI/Resource/RHITexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
	bool RenderTarget::InitializeBase( RHI::RHIDeviceRef& pDevice, ResourceCreateParameters const& createParams )
	{
        EE_ASSERT( pDevice );

        if ( m_isInitialized )
        {
            EE_LOG_WARNING( "Render", "Render Target", "Try to initialize render target twice!" );
            return false;
        }

        RenderTargetCreateParameters const& params = static_cast<RenderTargetCreateParameters const&>( createParams );

        RHI::RHITextureCreateDesc textureDesc = RHI::RHITextureCreateDesc::New2D(
            params.m_width, params.m_height, RHI::EPixelFormat::RGBA8Unorm
        );

        m_dimensions = Int2{ (int32_t) params.m_width, (int32_t) params.m_height };
        // TODO: add detection mechanism to make UI render target presentation compatible.
        textureDesc.m_usage.SetFlag( RHI::ETextureUsage::Sampled );
        m_pRenderTarget = pDevice->CreateTexture( textureDesc );
	
        if ( params.m_bNeedDepth )
        {
            textureDesc.m_format = RHI::EPixelFormat::Depth32;
            textureDesc.m_usage.ClearAllFlags();
            textureDesc.m_usage.SetFlag( RHI::ETextureUsage::DepthStencil );
            m_pDepthStencil = pDevice->CreateTexture( textureDesc );
        }

        if ( params.m_bNeedIdPicking )
        {
            textureDesc.m_format = RHI::EPixelFormat::RGBA32UInt;
            textureDesc.m_usage.ClearAllFlags();
            textureDesc.m_usage.SetMultipleFlags( RHI::ETextureUsage::Sampled );
            m_pPickingRT = pDevice->CreateTexture( textureDesc );

            textureDesc.m_width = 1;
            textureDesc.m_height = 1;
            textureDesc.m_usage.ClearAllFlags();
            textureDesc.m_usage.SetFlag( RHI::ETextureUsage::Storage );
            m_pPickingStagingRT = pDevice->CreateTexture( textureDesc );
        }

        m_isInitialized = true;
        return true;
    }

	void RenderTarget::Release( RHI::RHIDeviceRef& pDevice )
	{
        EE_ASSERT( pDevice );

        if ( !m_isInitialized )
        {
            return;
        }

        pDevice->DestroyTexture( m_pRenderTarget );

        if ( HasDepthStencil() )
        {
            pDevice->DestroyTexture( m_pDepthStencil );
        }

        if ( HasPickingRT() )
        {
            pDevice->DestroyTexture( m_pPickingRT );
            pDevice->DestroyTexture( m_pPickingStagingRT );
        }

        m_pRenderTarget = nullptr;
        m_pDepthStencil = nullptr;
        m_pPickingRT = nullptr;
        m_pPickingStagingRT = nullptr;

        m_isInitialized = false;
	}

    //-------------------------------------------------------------------------

	void SwapchainRenderTarget::Release( RHI::RHIDeviceRef& pDevice )
	{
        if ( m_isInitialized )
        {
            m_pRenderTarget = nullptr;
            m_pTextureAcquireSemaphore = nullptr;
            m_pRenderCompleteSemaphore = nullptr;

            m_pSwapchain.reset();
            m_isInitialized = false;
        }
	}

	bool SwapchainRenderTarget::InitializeBase( RHI::RHIDeviceRef& pDevice, ResourceCreateParameters const& createParams )
	{
        SwapchainRenderTargetCreateParameters const& params = static_cast<SwapchainRenderTargetCreateParameters const&>( createParams );
        m_pSwapchain = params.m_pSwapchain;

        m_dimensions = Int2{ (int32_t) m_pSwapchain->GetPresentTextureDesc().m_width, (int32_t) m_pSwapchain->GetPresentTextureDesc().m_height };

        if ( !m_pSwapchain )
        {
            return false;
        }

        m_isInitialized = true;
        return true;
	}

    //-------------------------------------------------------------------------

    void SwapchainRenderTarget::ResetFrame()
    {
        m_pRenderTarget = nullptr;
        m_pDepthStencil = nullptr;
        
        m_pTextureAcquireSemaphore = nullptr;
        m_pRenderCompleteSemaphore = nullptr;
        m_frameIndex = 0;
    }

	bool SwapchainRenderTarget::AcquireNextFrame()
	{
        EE_ASSERT( IsInitialized() );

        auto swapchainTexture = m_pSwapchain->AcquireNextFrameRenderTarget();

        m_frameIndex = swapchainTexture.m_frameIndex;
        m_pRenderTarget = swapchainTexture.m_pRenderTarget;
        m_pTextureAcquireSemaphore = swapchainTexture.m_pTextureAcquireSemaphore;
        m_pRenderCompleteSemaphore = swapchainTexture.m_pRenderCompleteSemaphore;

        return IsValid();
	}
}