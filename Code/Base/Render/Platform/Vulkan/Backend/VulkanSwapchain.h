#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Types/Event.h"
#include "Base/RHI/RHIObject.h"
#include "Base/RHI/RHISwapchain.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
        class VulkanTexture;
        class VulkanSemaphore;

		class VulkanSwapchain final : public RHI::RHISwapchain
		{
            EE_RHI_OBJECT( Vulkan, RHISwapchain )

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

                PFN_vkAcquireNextImageKHR                           m_pAcquireNextImageKHRFunc;
                PFN_vkQueuePresentKHR                               m_pQueuePresentKHR;
			};

		public:

            VulkanSwapchain( RHI::RHIDeviceRef& pDevice );
			VulkanSwapchain( InitConfig config, RHI::RHIDeviceRef& pDevice );

			~VulkanSwapchain();

        public:

            virtual bool Resize( Int2 const& dimensions ) override;

            virtual RHI::SwapchainTexture AcquireNextFrameRenderTarget() override;

            virtual void Present( RHI::SwapchainTexture&& swapchainRenderTarget ) override;

            virtual RHI::RHITextureCreateDesc GetPresentTextureDesc() const override;
            virtual TVector<RHI::RHITexture*> const GetPresentTextures() const override;

        private:

            bool CreateOrRecreate( InitConfig const& config, VkSwapchainKHR pOldSwapchain = nullptr );

            void OnTextrueDestroyed( RHI::RHITexture* pTexture );

            inline void AdvanceFrame() { m_currentRenderFrameIndex = ( m_currentRenderFrameIndex + 1 ) % static_cast<uint32_t>( m_presentTextures.size() ); }

		private:

            RHI::RHIDeviceRef           			m_pDevice;

			VkSwapchainKHR							m_pHandle;
			LoadFuncs								m_loadFuncs;
            InitConfig                              m_initConfig;

			TVector<VulkanTexture*>					m_presentTextures;
			TVector<VulkanSemaphore*>				m_textureAcquireSemaphores;
			TVector<VulkanSemaphore*>				m_renderCompleteSemaphores;

            uint32_t                                m_currentRenderFrameIndex;

            EventBindingID                          m_onSwapchainTextureDestroyedEventId;
		};
	}
}

#endif