#if defined(EE_VULKAN)
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        void VulkanDescriptorSetReleaseImpl::Release( RHI::RHIDevice* pDevice, void* pSetPool )
        {
            EE_ASSERT( pDevice );
            EE_ASSERT( pSetPool );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );
        
            vkDestroyDescriptorPool( pVkDevice->m_pHandle, reinterpret_cast<VkDescriptorPool>( pSetPool ), nullptr );
        }
    }
}

#endif