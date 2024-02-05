#if defined(EE_VULKAN)
#include "VulkanSynchronization.h"
#include "VulkanDevice.h"
#include "Base/RHI/RHIDowncastHelper.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        void* VulkanCPUGPUSyncImpl::Create( RHI::RHIDevice* pDevice, bool bSignaled )
        {
            EE_ASSERT( pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            VkFenceCreateInfo fenceCI = {};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCI.flags = bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

            VkFence fence;
            VK_SUCCEEDED( vkCreateFence( pVkDevice->m_pHandle, &fenceCI, nullptr, &fence ) );

            return fence;
        }

        void VulkanCPUGPUSyncImpl::Destroy( RHI::RHIDevice* pDevice, void*& pCPUGPUSync )
        {
            EE_ASSERT( pDevice );
            EE_ASSERT( pCPUGPUSync );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            vkDestroyFence( pVkDevice->m_pHandle, reinterpret_cast<VkFence>( pCPUGPUSync ), nullptr );
        }

        void VulkanCPUGPUSyncImpl::Reset( RHI::RHIDevice* pDevice, void* pCPUGPUSync )
        {
            EE_ASSERT( pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            VkFence fence = reinterpret_cast<VkFence>( pCPUGPUSync );
            VK_SUCCEEDED( vkResetFences( pVkDevice->m_pHandle, 1, &fence ) );
        }

        void VulkanCPUGPUSyncImpl::WaitFor( RHI::RHIDevice* pDevice, void* pCPUGPUSync, uint64_t waitTime )
        {
            EE_ASSERT( pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            VkFence fence = reinterpret_cast<VkFence>( pCPUGPUSync );
            VK_SUCCEEDED( vkWaitForFences( pVkDevice->m_pHandle, 1, &fence, true, waitTime ) );
        }
    }
}

#endif