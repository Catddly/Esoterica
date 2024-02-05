#if defined(EE_VULKAN)
#include "VulkanMemoryAllocator.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"

#include <vulkan/vulkan_core.h>

//-------------------------------------------------------------------------
// Disable specific warnings
//-------------------------------------------------------------------------

#pragma warning( push )
#pragma warning(disable:4324) // structure was padded to alignment specifier in VmaPoolAllcator

#if VULKAN_USE_VMA_ALLOCATION
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION
#endif

#pragma warning( pop )

namespace EE::Render
{
	namespace Backend
	{
        VulkanMemoryAllocator::~VulkanMemoryAllocator()
        {
            EE_ASSERT( m_pHandle == nullptr );
        }

        //-------------------------------------------------------------------------

		void VulkanMemoryAllocator::Initialize( VulkanDevice* pDevice )
		{
            m_pDevice = pDevice;

            EE_ASSERT( m_pHandle == nullptr );
            EE_ASSERT( m_pDevice != nullptr );

            VmaAllocatorCreateInfo allocatorCI = {};
            allocatorCI.instance = m_pDevice->m_pInstance->m_pHandle;
            allocatorCI.physicalDevice = m_pDevice->m_physicalDevice.m_pHandle;
            allocatorCI.device = m_pDevice->m_pHandle;
            allocatorCI.vulkanApiVersion = VK_MAKE_VERSION( 1, 3, 0 );

            VmaVulkanFunctions vulkanFunctions = {};
            vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
            vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
            vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
            vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
            vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
            vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
            vulkanFunctions.vkCreateImage = vkCreateImage;
            vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
            vulkanFunctions.vkDestroyImage = vkDestroyImage;
            vulkanFunctions.vkFreeMemory = vkFreeMemory;
            vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
            vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
            vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
            vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
            vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
            vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
            vulkanFunctions.vkMapMemory = vkMapMemory;
            vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
            vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
            vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
            vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

            allocatorCI.pVulkanFunctions = &vulkanFunctions;
            allocatorCI.pAllocationCallbacks = nullptr;

            VK_SUCCEEDED( vmaCreateAllocator( &allocatorCI, &m_pHandle ) );
		}

        void VulkanMemoryAllocator::Shutdown()
        {
            EE_ASSERT( m_pHandle != nullptr );

            vmaDestroyAllocator( m_pHandle );
            m_pHandle = nullptr;
        }
	}
}

#endif