#if defined(EE_VULKAN)
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        void VulkanDescriptorSetReleaseImpl::Release( RHI::RHIDeviceRef& pDevice, void* pSetPool )
        {
            EE_ASSERT( pSetPool );
            auto pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
        
            vkDestroyDescriptorPool( pVkDevice->m_pHandle, reinterpret_cast<VkDescriptorPool>( pSetPool ), nullptr );
        }
    }
}

#endif