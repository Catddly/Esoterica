#include "RenderTarget.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/RHISwapchain.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"
#include "Base/RHI/Resource/RHITexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    Int2 RenderTarget::GetDimensions() const
    {
        EE_ASSERT( IsValid() );
        return Int2{ 
            static_cast<int>( m_pRenderTarget->GetDesc().m_width ),
            static_cast<int>( m_pRenderTarget->GetDesc().m_height )
        };
    }

    //-------------------------------------------------------------------------

	bool RenderTarget::InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams )
	{
        EE_ASSERT( pDevice );

        RenderTargetCreateParameters const& params = static_cast<RenderTargetCreateParameters const&>( createParams );

        RHI::RHITextureCreateDesc textureDesc = RHI::RHITextureCreateDesc::New2D(
            params.m_width, params.m_height, RHI::EPixelFormat::RGBA8Unorm
        );

        m_pRenderTarget = pDevice->CreateTexture( textureDesc );
	
        if ( params.m_bNeedDepth )
        {
            textureDesc.m_format = RHI::EPixelFormat::Depth32;
            textureDesc.m_usage = RHI::ETextureUsage::DepthStencil;
            m_pDepthStencil = pDevice->CreateTexture( textureDesc );
        }

        if ( params.m_bNeedIdPicking )
        {
            textureDesc.m_format = RHI::EPixelFormat::RGBA32UInt;
            textureDesc.m_usage.SetMultipleFlags( RHI::ETextureUsage::Sampled, RHI::ETextureUsage::Storage );
            m_pPickingRT = pDevice->CreateTexture( textureDesc );

            textureDesc.m_width = 1;
            textureDesc.m_height = 1;
            textureDesc.m_usage.ClearAllFlags();
            textureDesc.m_usage.SetFlag( RHI::ETextureUsage::Transient );
            m_pPickingStagingRT = pDevice->CreateTexture( textureDesc );
        }

        return true;
    }

	void RenderTarget::Release( RHI::RHIDevice* pDevice )
	{
        EE_ASSERT( pDevice );

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
	}

    //-------------------------------------------------------------------------

	bool SwapchainRenderTarget::InitializeBase( RHI::RHIDevice* pDevice, ResourceCreateParameters const& createParams )
	{
        SwapchainRenderTargetCreateParameters const& params = static_cast<SwapchainRenderTargetCreateParameters const&>( createParams );
        m_pSwapchain = params.m_pSwapchain;

        if ( !m_pSwapchain )
        {
            return false;
        }

        m_isInitialized = true;
        return true;
	}

	void SwapchainRenderTarget::Release( RHI::RHIDevice* pDevice )
	{
        if ( m_pSwapchain )
        {
            m_pRenderTarget = nullptr;
            m_pTextureAcquireSemaphore = nullptr;
            m_pRenderCompleteSemaphore = nullptr;

            m_pSwapchain = nullptr;
            m_isInitialized = false;
        }
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