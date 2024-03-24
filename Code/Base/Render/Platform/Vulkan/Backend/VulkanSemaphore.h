#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHISemaphore.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		class VulkanSemaphore final : public RHI::RHISemaphore
		{
            EE_RHI_OBJECT( Vulkan, RHISemaphore )

			friend class VulkanDevice;
			friend class VulkanSwapchain;
            friend class VulkanCommandQueue;

		public:

            VulkanSemaphore()
                : RHISemaphore( RHI::ERHIType::Vulkan )
            {}

		private:

			VkSemaphore						m_pHandle = nullptr;
		};
	}
}

#endif