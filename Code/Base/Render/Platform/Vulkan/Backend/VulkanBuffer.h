#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "VulkanCommon.h"

#include <vulkan/vulkan_core.h>
#if VULKAN_USE_VMA_ALLOCATION
#include <vma/vk_mem_alloc.h>
#endif

namespace EE::Render
{
    namespace Backend
    {
        class VulkanBuffer : public RHI::RHIBuffer
        {
            EE_RHI_OBJECT( Vulkan, RHIBuffer )

            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            VulkanBuffer()
                : RHIBuffer( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanBuffer() = default;

        public:

            [[nodiscard]] virtual void* Map( RHI::RHIDeviceRef& pDevice) override;
            virtual void Unmap( RHI::RHIDeviceRef& pDevice ) override;

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