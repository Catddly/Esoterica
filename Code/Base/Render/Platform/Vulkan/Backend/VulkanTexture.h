#pragma once
#if defined(EE_VULKAN)

#include "Base/Math/Math.h"
#include "Base/Types/Map.h"
#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHITexture.h"
#include "VulkanCommon.h"

#include <vulkan/vulkan_core.h>
#if VULKAN_USE_VMA_ALLOCATION
#include <vma/vk_mem_alloc.h>
#endif

namespace EE::RHI
{
    class RHIBuffer;
}

namespace EE::Render
{
	namespace Backend
    {
        class VulkanTextureView
        {
            friend class VulkanDevice;
            friend class VulkanTexture;
            friend class VulkanCommandBuffer;

        private:

            VkImageView						        m_pHandle = nullptr;
        };

		class VulkanTexture : public RHI::RHITexture
		{
            EE_RHI_OBJECT( Vulkan, RHITexture )

            friend class VulkanDevice;
			friend class VulkanSwapchain;
			friend class VulkanCommandBuffer;

		public:

            VulkanTexture()
                : RHITexture( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanTexture() = default;

        public:

            virtual void* MapSlice( RHI::RHIDeviceRef& pDevice, uint32_t layer ) override;
            virtual void  UnmapSlice( RHI::RHIDeviceRef& pDevice, uint32_t layer ) override;

            // Upload mapped slice from CPU to GPU texture.
            // Return true if no data to upload or update data successfully.
            virtual bool UploadMappedData( RHI::RHIDeviceRef& pDevice, RHI::RenderResourceBarrierState dstBarrierState ) override;

        private:

            void ForceDiscardAllUploadedData( RHI::RHIDeviceRef& pDevice );

            virtual RHI::RHITextureView  CreateView( RHI::RHIDeviceRef& pDevice, RHI::RHITextureViewCreateDesc const& desc) const override;
            virtual void                 DestroyView( RHI::RHIDeviceRef& pDevice, RHI::RHITextureView& textureView ) override;

            uint32_t GetLayerByteSize( uint32_t layer );

		private:

			VkImage							    m_pHandle = nullptr;
            #if VULKAN_USE_VMA_ALLOCATION
            VmaAllocation                       m_allocation = nullptr;
            #else
            VkDeviceMemory                      m_allocation = nullptr;
            #endif // VULKAN_USE_VMA_ALLOCATION

            TMap<uint32_t, RHI::RHIBuffer*>     m_waitToFlushMappedMemory;
		};
    }
}

#endif