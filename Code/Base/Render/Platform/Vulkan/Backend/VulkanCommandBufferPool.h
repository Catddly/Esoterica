#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Render/RenderAPI.h"

#include <vulkan/vulkan_core.h>
#include <limits>

namespace EE::RHI
{
    class RHISemaphore;
}

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

            void SubmitToQueue( VulkanCommandBuffer* pCommandBuffer, TSpan<RHI::RHISemaphore*> pWaitSemaphores, TSpan<RHI::RHISemaphore*> pSignalSemaphores, TSpan<Render::PipelineStage> waitStages );

            bool IsReadyToAllocate() const { return m_bReadyToAllocate; }

            // Wait until all commands inside this command buffer pool are finished.
            void WaitUntilAllCommandsFinish();

            // Reset whole command buffer pool for next command allocation and record.
            // After reset, IsReadyToAllocate() will return true. Otherwise false.
            void Reset();

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
        
            bool                                    m_bReadyToAllocate = false;
        };
    }
}

#endif