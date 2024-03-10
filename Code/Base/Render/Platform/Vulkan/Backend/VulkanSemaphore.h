#pragma once
#ifdef EE_VULKAN

#include "Base/RHI/Resource/RHISemaphore.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		class VulkanSemaphore final : public RHI::RHISemaphore
		{
			friend class VulkanDevice;
			friend class VulkanSwapchain;
            friend class VulkanCommandQueue;

		public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanSemaphore()
                : RHISemaphore( RHI::ERHIType::Vulkan )
            {}

		private:

			VkSemaphore						m_pHandle = nullptr;
		};
	}
}

#endif