#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/Resource/RHIBuffer.h"
#include "VulkanCommon.h"

#include <vulkan/vulkan_core.h>
#if VULKAN_USE_VMA_ALLOCATION
#include <vma/vk_mem_alloc.h>
#endif

namespace EE::RHI
{
    class RHIDevice;
}

namespace EE::Render
{
    namespace Backend
    {
        //struct VulkanBufferReleaseImpl : public RHI::IRHIBufferReleaseImpl
        //{
        //    inline virtual void Release( RHI::RHIDevice* pDevice, void* pBuffer ) override;
        //};

        class VulkanBuffer : public RHI::RHIBuffer
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanBuffer()
                : RHIBuffer( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanBuffer() = default;

        public:

            virtual void* Map( RHI::RHIDevice* pDevice ) override;
            virtual void  Unmap( RHI::RHIDevice* pDevice ) override;

        private:

            VkBuffer					    m_pHandle = nullptr;
            #if VULKAN_USE_VMA_ALLOCATION
            VmaAllocation                   m_allocation = nullptr;
            #else
            VkDeviceMemory                  m_allocation = nullptr;
            #endif // VULKAN_USE_VMA_ALLOCATION

            void*                           m_pMappedMemory = nullptr;
        };
    }
}

#endif