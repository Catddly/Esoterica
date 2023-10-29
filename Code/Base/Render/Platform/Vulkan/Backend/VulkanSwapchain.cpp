#if defined(EE_VULKAN)
#include "VulkanSwapchain.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanSemaphore.h"
#include "VulkanUtils.h"
#include "RHIToVulkanSpecification.h"
#include "Base/Logging/Log.h"
#include "Base/RHI/Resource/RHISemaphore.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::Render
{
	namespace Backend
	{
		VulkanSwapchain::InitConfig VulkanSwapchain::InitConfig::GetDefault()
		{
			Int2 extent = Util::GetCurrentActiveWindowUserExtent();

			InitConfig config = {};
			config.m_width = static_cast<uint32_t>( extent.m_x );
			config.m_height = static_cast<uint32_t>( extent.m_y );
			config.m_enableVsync = false;
			return config;
		}

		//-------------------------------------------------------------------------

		VulkanSwapchain::VulkanSwapchain( VulkanDevice* pDevice )
			: VulkanSwapchain( InitConfig::GetDefault(), pDevice )
		{}

		VulkanSwapchain::VulkanSwapchain( InitConfig config, VulkanDevice* pDevice )
			: RHISwapchain( RHI::ERHIType::Vulkan ), m_pDevice( pDevice )
		{
            // platform specific
            //-------------------------------------------------------------------------

            if ( config.m_format == RHI::EPixelFormat::RGBA8Unorm )
            {
                config.m_format = RHI::EPixelFormat::BGRA8Unorm;
            }

			// load function
			//-------------------------------------------------------------------------

			m_loadFuncs.m_pGetPhysicalDeviceSurfaceCapabilitiesKHRFunc = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) m_pDevice->m_pInstance->GetProcAddress( "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
			m_loadFuncs.m_pGetPhysicalDeviceSurfaceFormatsKHRFunc = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) m_pDevice->m_pInstance->GetProcAddress( "vkGetPhysicalDeviceSurfaceFormatsKHR" );
			m_loadFuncs.m_pGetPhysicalDeviceSurfacePresentModesKHRFunc = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) m_pDevice->m_pInstance->GetProcAddress( "vkGetPhysicalDeviceSurfacePresentModesKHR" );
			m_loadFuncs.m_pCreateSwapchainKHRFunc = (PFN_vkCreateSwapchainKHR) m_pDevice->m_pInstance->GetProcAddress( "vkCreateSwapchainKHR" );
			m_loadFuncs.m_pDestroySwapchainKHRFunc = (PFN_vkDestroySwapchainKHR) m_pDevice->m_pInstance->GetProcAddress( "vkDestroySwapchainKHR" );
			m_loadFuncs.m_pGetSwapchainImagesKHRFunc = (PFN_vkGetSwapchainImagesKHR) m_pDevice->m_pInstance->GetProcAddress( "vkGetSwapchainImagesKHR" );

			// pick swapchain format and color space
			// TODO: support HDR display device
			//-------------------------------------------------------------------------

			EE_ASSERT( m_loadFuncs.m_pGetPhysicalDeviceSurfaceFormatsKHRFunc != nullptr );

			uint32_t surfaceFormatCount = 0;
			VK_SUCCEEDED( m_loadFuncs.m_pGetPhysicalDeviceSurfaceFormatsKHRFunc( m_pDevice->m_physicalDevice.m_pHandle, m_pDevice->m_pSurface->m_pHandle, &surfaceFormatCount, nullptr ) );
			TVector<VkSurfaceFormatKHR> surfaceFormats( surfaceFormatCount );
			VK_SUCCEEDED( m_loadFuncs.m_pGetPhysicalDeviceSurfaceFormatsKHRFunc( m_pDevice->m_physicalDevice.m_pHandle, m_pDevice->m_pSurface->m_pHandle, &surfaceFormatCount, surfaceFormats.data() ) );

			if ( surfaceFormatCount == 0 )
			{
				EE_LOG_FATAL_ERROR( "Render", "Vulkan Backend", "Surface support zero valid format!" );
			}

			VkSurfaceFormatKHR pickFormat = {};
            VkFormat desireFormat = ToVulkanFormat( config.m_format );
            RHI::EPixelFormat pickedRhiFormat = config.m_format;

			if ( surfaceFormatCount == 1 )
			{
				if ( surfaceFormats[0].format == VK_FORMAT_UNDEFINED )
				{
					pickFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
					pickFormat.colorSpace = surfaceFormats[0].colorSpace;
                    pickedRhiFormat = RHI::EPixelFormat::BGRA8Unorm;
				}
			}
			else
			{
				for ( auto const& format : surfaceFormats )
				{
					if ( format.format == desireFormat )
					{
						pickFormat.format = desireFormat;
                        pickedRhiFormat = config.m_format;
						break;
					}
				}
			}

			// get image count and extent
			//-------------------------------------------------------------------------

			VkSurfaceCapabilitiesKHR surfaceCaps = {};
			EE_ASSERT( m_loadFuncs.m_pGetPhysicalDeviceSurfaceCapabilitiesKHRFunc != nullptr );
			VK_SUCCEEDED( m_loadFuncs.m_pGetPhysicalDeviceSurfaceCapabilitiesKHRFunc( m_pDevice->m_physicalDevice.m_pHandle, m_pDevice->m_pSurface->m_pHandle, &surfaceCaps ) );
		
			uint32_t const imageCount = Math::Max( config.m_swapBufferCount, surfaceCaps.minImageCount );
			if ( imageCount > surfaceCaps.maxImageCount )
			{
				EE_LOG_FATAL_ERROR( "Render", "Vulkan Backend", "Vulkan swapchain image count exceed max image count limit: %u", surfaceCaps.maxImageCount );
			}

			Int2 extent = Int2::Zero;

			if ( surfaceCaps.currentExtent.width != std::numeric_limits<uint32_t>::max() )
			{
				extent.m_x = surfaceCaps.currentExtent.width;
                //extent.m_x = config.m_width;
			}
			if ( surfaceCaps.currentExtent.height != std::numeric_limits<uint32_t>::max() )
			{
				extent.m_y = surfaceCaps.currentExtent.height;
                //extent.m_y = config.m_height;
			}

			EE_ASSERT( extent != Int2::Zero );

			// get present mode
			//-------------------------------------------------------------------------

			EE_ASSERT( m_loadFuncs.m_pGetPhysicalDeviceSurfacePresentModesKHRFunc != nullptr );

			uint32_t supportedPresentModeCount = 0;
			m_loadFuncs.m_pGetPhysicalDeviceSurfacePresentModesKHRFunc( pDevice->m_physicalDevice.m_pHandle, pDevice->m_pSurface->m_pHandle, &supportedPresentModeCount, nullptr );
			TVector<VkPresentModeKHR> supportedPresentModes( supportedPresentModeCount );
			m_loadFuncs.m_pGetPhysicalDeviceSurfacePresentModesKHRFunc( pDevice->m_physicalDevice.m_pHandle, pDevice->m_pSurface->m_pHandle, &supportedPresentModeCount, supportedPresentModes.data() );

			// choose present modes by vsync, the one at the front will be chosen first if they both supported by the surface.
			// more info: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
			TVector<VkPresentModeKHR> presentModes;
			if ( config.m_enableVsync )
			{
				presentModes = { VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR };
			}
			else
			{
				presentModes = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR };
			}

			VkPresentModeKHR pickPresentMode = VK_PRESENT_MODE_FIFO_KHR;
			for ( auto const& pm : presentModes )
			{
				if ( VectorContains( supportedPresentModes, pm ) )
				{
					pickPresentMode = pm;
				}
			}

			// get surface transform
			//-------------------------------------------------------------------------

			VkSurfaceTransformFlagBitsKHR transformFlag = {};
			if ( (surfaceCaps.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) != 0 )
			{
				transformFlag = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			}
			else
			{
				transformFlag = surfaceCaps.currentTransform;
			}

			// create swapchain
			//-------------------------------------------------------------------------

			VkSwapchainCreateInfoKHR swapchainCI = {};
			swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCI.flags = VkFlags( 0 );
			swapchainCI.pNext = nullptr;
			swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCI.presentMode = pickPresentMode;
			swapchainCI.clipped = true;
			swapchainCI.preTransform = transformFlag;
			swapchainCI.surface = m_pDevice->m_pSurface->m_pHandle;
			swapchainCI.minImageCount = imageCount;

			swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCI.imageUsage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainCI.imageExtent = { (uint32_t) extent.m_x, (uint32_t) extent.m_y };
			swapchainCI.imageFormat = pickFormat.format;
			swapchainCI.imageColorSpace = pickFormat.colorSpace;
			swapchainCI.imageArrayLayers = 1;

			EE_ASSERT( m_loadFuncs.m_pCreateSwapchainKHRFunc != nullptr );
			VK_SUCCEEDED( m_loadFuncs.m_pCreateSwapchainKHRFunc( m_pDevice->m_pHandle, &swapchainCI, nullptr, &m_pHandle ) );

			// fetch swapchain images
			//-------------------------------------------------------------------------

			uint32_t swapchainImageCount = 0;
			VK_SUCCEEDED( m_loadFuncs.m_pGetSwapchainImagesKHRFunc( m_pDevice->m_pHandle, m_pHandle, &swapchainImageCount, nullptr ) );
			TVector<VkImage> swapchainImages( swapchainImageCount );
			VK_SUCCEEDED( m_loadFuncs.m_pGetSwapchainImagesKHRFunc( m_pDevice->m_pHandle, m_pHandle, &swapchainImageCount, swapchainImages.data() ) );

			for ( uint32_t i = 0; i < swapchainImageCount; ++i )
			{
                auto desc = RHI::RHITextureCreateDesc::New2D( extent.m_x, extent.m_y, pickedRhiFormat );
                desc.m_usage.SetMultipleFlags( RHI::ETextureUsage::Storage, RHI::ETextureUsage::Color );

				auto* pImage = EE::New<VulkanTexture>();
                pImage->m_pHandle = swapchainImages[i];
                pImage->m_desc = std::move( desc );

				m_presentTextures.push_back( pImage );
			}

			EE_ASSERT( m_presentTextures.size() == swapchainImages.size() );

			// create semaphores
			//-------------------------------------------------------------------------

			m_textureAcquireSemaphores.resize( swapchainImageCount );
			m_renderCompleteSemaphores.resize( swapchainImageCount );
			for ( uint32_t i = 0; i < swapchainImageCount; ++i )
			{
                m_textureAcquireSemaphores[i] = static_cast<VulkanSemaphore*>( pDevice->CreateSyncSemaphore( RHI::RHISemaphoreCreateDesc{} ) );
                m_renderCompleteSemaphores[i] = static_cast<VulkanSemaphore*>( pDevice->CreateSyncSemaphore( RHI::RHISemaphoreCreateDesc{} ) );
			}

			EE_ASSERT( m_textureAcquireSemaphores.size() == swapchainImages.size() );
			EE_ASSERT( m_renderCompleteSemaphores.size() == swapchainImages.size() );
		}

		VulkanSwapchain::~VulkanSwapchain()
		{
			for ( int i = (int)m_textureAcquireSemaphores.size() - 1; i >= 0; i-- )
			{
                m_pDevice->DestroySyncSemaphore( m_textureAcquireSemaphores[i] );
                m_pDevice->DestroySyncSemaphore( m_renderCompleteSemaphores[i] );
			}

			m_textureAcquireSemaphores.clear();
			m_renderCompleteSemaphores.clear();

            for ( int i = (int) m_presentTextures.size() - 1; i >= 0; i-- )
            {
                auto* pVkTexture = m_presentTextures[i];
                EE::Delete( pVkTexture );
            }

			EE_ASSERT( m_pHandle != nullptr );
			EE_ASSERT( m_loadFuncs.m_pDestroySwapchainKHRFunc != nullptr );

			m_loadFuncs.m_pDestroySwapchainKHRFunc( m_pDevice->m_pHandle, m_pHandle, nullptr );
			m_pHandle = nullptr;
		}

        //-------------------------------------------------------------------------

		RHI::RHITextureCreateDesc VulkanSwapchain::GetPresentTextureDesc() const
        {
            EE_ASSERT( !m_presentTextures.empty() );
            return m_presentTextures[0]->m_desc;
        }

		TVector<RHI::RHITexture const*> const VulkanSwapchain::GetPresentTextures() const
		{
            TVector<RHI::RHITexture const*> textures;
            textures.resize( m_presentTextures.size() );
            
            for ( uint32_t i = 0; i < m_presentTextures.size(); ++i )
            {
                textures[i] = m_presentTextures[i];
            }

            return textures;
		}
	}
}

#endif