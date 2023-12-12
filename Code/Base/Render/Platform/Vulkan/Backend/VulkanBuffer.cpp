#if defined(EE_VULKAN)
#include "VulkanBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        void* VulkanBuffer::Map( RHI::RHIDevice* pDevice )
        {
            if ( m_desc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::GPUOnly ||
                 m_desc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::GPULazily )
            {
                return nullptr;
            }

            EE_ASSERT( pDevice );
            EE_ASSERT( m_pHandle );

            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            #if VULKAN_USE_VMA_ALLOCATION
            if ( m_desc.m_memoryFlag.IsFlagSet( RHI::ERenderResourceMemoryFlag::PersistentMapping ) )
            {
                return m_pMappedMemory;
            }

            VmaAllocator vmaAllocator = pVkDevice->m_globalMemoryAllcator.m_pHandle;
            VK_SUCCEEDED( vmaMapMemory( vmaAllocator, m_allocation, &m_pMappedMemory ) );
            #else
            VK_SUCCEEDED( vkMapMemory( pVkDevice->m_pHandle, m_allocation,
                          0, &m_pMappedMemory ));
                          0, static_cast<VkDeviceSize>( m_desc.m_desireSize ),
            #endif

            return m_pMappedMemory;
        }

        void VulkanBuffer::Unmap( RHI::RHIDevice* pDevice )
        {
            if ( m_desc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::GPUOnly ||
                 m_desc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::GPULazily )
            {
                return;
            }

            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            EE_ASSERT( pVkDevice );

            #if VULKAN_USE_VMA_ALLOCATION
            vmaUnmapMemory( pVkDevice->m_globalMemoryAllcator.m_pHandle, m_allocation );
            #else
            vkUnmapMemory( pVkDevice->m_pHandle, m_allocation );
            #endif

            #if VULKAN_USE_VMA_ALLOCATION
            if ( !m_desc.m_memoryFlag.IsFlagSet( RHI::ERenderResourceMemoryFlag::PersistentMapping ) )
            {
                m_pMappedMemory = nullptr;
            }
            #endif
        }
    }
}
#endif