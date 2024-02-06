#if defined(EE_VULKAN)
#include "VulkanCommandQueue.h"
#include "VulkanDevice.h"

namespace EE::Render
{
    namespace Backend
    {
        VulkanCommandQueue::VulkanCommandQueue( VulkanDevice const& device, RHI::CommandQueueType type, QueueFamily const& queueFamily, uint32_t queueIndex )
            : RHI::RHICommandQueue( RHI::ERHIType::Vulkan ), m_queueFamily( queueFamily ), m_queueIndex( queueIndex )
        {
            EE_ASSERT( queueIndex < queueFamily.m_props.queueCount );

            vkGetDeviceQueue( device.m_pHandle, queueFamily.m_index, queueIndex, &m_pHandle );

            if ( type == RHI::CommandQueueType::Graphic && queueFamily.IsGraphicQueue() )
            {
                m_type = RHI::CommandQueueType::Graphic;
            }
            else if ( type == RHI::CommandQueueType::Compute && queueFamily.IsComputeQueue() )
            {
                m_type = RHI::CommandQueueType::Compute;
            }
            else if ( type == RHI::CommandQueueType::Transfer && queueFamily.IsTransferQueue() )
            {
                m_type = RHI::CommandQueueType::Transfer;
            }

            EE_ASSERT( m_pHandle != nullptr );
        }

        void VulkanCommandQueue::WaitUntilIdle() const
        {
            vkQueueWaitIdle( m_pHandle );
        }
	}
}

#endif