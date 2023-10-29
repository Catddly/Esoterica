#pragma once
#if defined(EE_VULKAN)

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanCommandQueue;

        class VulkanCommandBufferPool
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            VulkanCommandBufferPool() = default;

            VulkanCommandBufferPool( VulkanCommandBufferPool const& ) = delete;
            VulkanCommandBufferPool& operator=( VulkanCommandBufferPool const& ) = delete;

            VulkanCommandBufferPool( VulkanCommandBufferPool&& ) = delete;
            VulkanCommandBufferPool& operator=( VulkanCommandBufferPool&& ) = delete;

            //-------------------------------------------------------------------------

        private:

            VkCommandPool               m_pHandle = nullptr;
            VulkanCommandQueue*         m_pCommandQueue = nullptr;
        };
    }
}

#endif