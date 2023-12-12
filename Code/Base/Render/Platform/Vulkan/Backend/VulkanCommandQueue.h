#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/RHICommandQueue.h"
#include "VulkanPhysicalDevice.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanCommandQueue : public RHI::RHICommandQueue
        {
            friend class VulkanDevice;
            friend class VulkanSwapchain;
            friend class VulkanCommandBuffer;
            friend class VulkanCommandBufferPool;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanCommandQueue()
                : RHI::RHICommandQueue( RHI::ERHIType::Vulkan )
            {}
            VulkanCommandQueue( VulkanDevice const& device, QueueFamily const& queueFamily );
            virtual ~VulkanCommandQueue() = default;

        public:

            inline bool IsValid() const { return m_pHandle != nullptr; }

            inline virtual uint32_t GetDeviceIndex() const override { return m_queueFamily.m_index; }
            inline virtual RHI::CommandQueueType GetType() const override { return m_type; }

        private:

            VkQueue								m_pHandle = nullptr;
            RHI::CommandQueueType               m_type = RHI::CommandQueueType::Unknown;
            QueueFamily							m_queueFamily;
        };
    }
}

#endif