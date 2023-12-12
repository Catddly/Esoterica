#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"

#include <vulkan/vulkan_core.h>
#include <limits>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanDevice;

        class VulkanCommandBuffer;
        class VulkanCommandQueue;

        class VulkanCommandBufferPool
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

            constexpr static uint32_t NumMaxCommandBufferPerPool = 8;

            using AllocatedCommandBufferArray = TVector<TFixedVector<VulkanCommandBuffer*, NumMaxCommandBufferPerPool>>;

        public:

            VulkanCommandBufferPool( VulkanDevice* pDevice, VulkanCommandQueue* pCommandQueue );
            ~VulkanCommandBufferPool();

            VulkanCommandBufferPool( VulkanCommandBufferPool const& ) = delete;
            VulkanCommandBufferPool& operator=( VulkanCommandBufferPool const& ) = delete;

            VulkanCommandBufferPool( VulkanCommandBufferPool&& ) = delete;
            VulkanCommandBufferPool& operator=( VulkanCommandBufferPool&& ) = delete;

            //-------------------------------------------------------------------------

            VulkanCommandBuffer* Allocate();

            void SubmitToQueue( VulkanCommandBuffer* pCommandBuffer );

            void Reset();

            void WaitUntilAllCommandsFinish();

        private:

            void CreateNewPool();

            VulkanCommandBuffer* FindIdleCommandBuffer();

            inline uint32_t GetPoolAllocatedBufferCount() const;

            inline void WaitAllFences( TInlineVector<VkFence, 16> fences, uint64_t timeout = std::numeric_limits<uint64_t>::max() );

        private:

            VulkanDevice*                           m_pDevice = nullptr;
            VulkanCommandQueue*                     m_pCommandQueue = nullptr;

            TVector<VkCommandPool>                  m_poolHandles;

            AllocatedCommandBufferArray             m_allocatedCommandBuffers;
            TVector<VulkanCommandBuffer*>           m_submittedCommandBuffers;
            TVector<VulkanCommandBuffer*>           m_idleCommandBuffers;
        };
    }
}

#endif