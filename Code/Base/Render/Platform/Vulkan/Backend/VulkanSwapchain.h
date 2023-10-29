#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/RHI/RHISwapchain.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		class VulkanDevice;
        class VulkanTexture;
        class VulkanSemaphore;

		class VulkanSwapchain final : public RHI::RHISwapchain
		{
		public:

			struct InitConfig
			{
				static InitConfig GetDefault();

				bool				m_enableVsync;
				uint32_t			m_width;
				uint32_t			m_height;
                uint32_t            m_swapBufferCount;
                RHI::EPixelFormat   m_format = RHI::EPixelFormat::Undefined;
                RHI::ESampleCount   m_sample;

			};

			struct LoadFuncs
			{
				PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR		m_pGetPhysicalDeviceSurfaceCapabilitiesKHRFunc;
				PFN_vkGetPhysicalDeviceSurfaceFormatsKHR			m_pGetPhysicalDeviceSurfaceFormatsKHRFunc;
				PFN_vkGetPhysicalDeviceSurfacePresentModesKHR		m_pGetPhysicalDeviceSurfacePresentModesKHRFunc;
				PFN_vkCreateSwapchainKHR							m_pCreateSwapchainKHRFunc;
				PFN_vkDestroySwapchainKHR							m_pDestroySwapchainKHRFunc;
				PFN_vkGetSwapchainImagesKHR							m_pGetSwapchainImagesKHRFunc;
			};

		public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )
			
            VulkanSwapchain( VulkanDevice* pDevice );
			VulkanSwapchain( InitConfig config, VulkanDevice* pDevice );

			~VulkanSwapchain();

        public:

            virtual RHI::RHITextureCreateDesc GetPresentTextureDesc() const override;
            virtual TVector<RHI::RHITexture const*> const GetPresentTextures() const override;

		private:

            // Cache device pointer (Use TSharedPtr ?)
			VulkanDevice*           				m_pDevice = nullptr;

			VkSwapchainKHR							m_pHandle;
			LoadFuncs								m_loadFuncs;

			TVector<VulkanTexture*>					m_presentTextures;
			TVector<VulkanSemaphore*>				m_textureAcquireSemaphores;
			TVector<VulkanSemaphore*>				m_renderCompleteSemaphores;
		};
	}
}

#endif