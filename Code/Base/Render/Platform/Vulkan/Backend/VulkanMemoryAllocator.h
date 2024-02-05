#pragma once
#if defined(EE_VULKAN)

#include "Base/Memory/Pointers.h"
#include "VulkanCommon.h"

#if VULKAN_USE_VMA_ALLOCATION
#include <vma/vk_mem_alloc.h>
#endif

namespace EE::Render
{
	namespace Backend
	{
        class VulkanDevice;

		class VulkanMemoryAllocator
		{
            friend class VulkanDevice;
            friend class VulkanBuffer;

		public:

			struct InitConfig
			{

			};

		public:

            VulkanMemoryAllocator() = default;
            ~VulkanMemoryAllocator();

            VulkanMemoryAllocator( VulkanMemoryAllocator const& ) = delete;
            VulkanMemoryAllocator& operator=( VulkanMemoryAllocator const& ) = delete;

            VulkanMemoryAllocator( VulkanMemoryAllocator&& ) = delete;
            VulkanMemoryAllocator& operator=( VulkanMemoryAllocator&& ) = delete;

            //-------------------------------------------------------------------------

            void Initialize( VulkanDevice* pDevice );
            void Shutdown();

		private:

            VulkanDevice*               m_pDevice = nullptr;

            #if VULKAN_USE_VMA_ALLOCATION
			VmaAllocator				m_pHandle = nullptr;
            #endif
		};
	}
}

#endif