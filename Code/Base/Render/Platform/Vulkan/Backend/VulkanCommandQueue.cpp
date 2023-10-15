#if defined(EE_VULKAN)
#include "VulkanCommandQueue.h"
#include "VulkanDevice.h"

namespace EE::Render
{
    namespace Backend
    {
        VulkanCommandQueue::VulkanCommandQueue( VulkanDevice const& device, QueueFamily const& queueFamily )
            : RHI::RHICommandQueue( RHI::ERHIType::Vulkan ), m_queueFamily( queueFamily )
        {
            vkGetDeviceQueue( device.m_pHandle, queueFamily.m_index, 0, &m_pHandle );

            if ( queueFamily.IsGraphicQueue() )
            {
                m_type = RHI::CommandQueueType::Graphic;
            }
            else if ( queueFamily.IsComputeQueue() )
            {
                m_type = RHI::CommandQueueType::Compute;
            }
            else if ( queueFamily.IsTransferQueue() )
            {
                m_type = RHI::CommandQueueType::Transfer;
            }

            EE_ASSERT( m_pHandle != nullptr );
        }
	}
}

#endif