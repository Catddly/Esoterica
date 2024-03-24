#pragma once
#if defined(EE_VULKAN)

#include "Base/Threading/Threading.h"
#include "Base/RHI/RHIObject.h"
#include "Base/RHI/RHICommandQueue.h"
#include "VulkanPhysicalDevice.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanCommandQueue : public RHI::RHICommandQueue
        {
            EE_RHI_OBJECT( Vulkan, RHICommandQueue )

            friend class VulkanDevice;
            friend class VulkanSwapchain;
            friend class VulkanCommandBuffer;
            friend class VulkanCommandBufferPool;

        public:

            VulkanCommandQueue()
                : RHI::RHICommandQueue( RHI::ERHIType::Vulkan )
            {}
            VulkanCommandQueue( VulkanDevice* pDevice, RHI::CommandQueueType type, QueueFamily const& queueFamily, uint32_t queueIndex );
            virtual ~VulkanCommandQueue() = default;

        public:

            inline bool IsValid() const { return m_pHandle != nullptr; }

            inline virtual uint32_t GetQueueIndex() const override { return m_queueFamily.m_index; }
            inline virtual RHI::CommandQueueType GetType() const override { return m_type; }

            inline virtual void WaitUntilIdle() const override;

            //virtual void AddTransferCommandSyncPoint() override;

            virtual void FlushToGPU() override;

        private:

            VulkanDevice*                       m_pDevice = nullptr;

            VkQueue								m_pHandle = nullptr;
            RHI::CommandQueueType               m_type = RHI::CommandQueueType::Unknown;
            QueueFamily							m_queueFamily;
            uint32_t                            m_queueIndex = 0;
        };
    }
}

#endif