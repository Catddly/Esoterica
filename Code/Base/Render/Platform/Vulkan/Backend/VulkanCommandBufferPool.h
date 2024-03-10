#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Threading/Threading.h"
#include "Base/RHI/RHICommandBufferPool.h"

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

        class VulkanCommandBufferPool final : public RHI::RHICommandBufferPool
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

            constexpr static uint32_t NumMaxCommandBufferPerPool = 32;

            using AllocatedCommandBufferArray = TVector<TFixedVector<VulkanCommandBuffer*, NumMaxCommandBufferPerPool>>;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanCommandBufferPool()
                : RHI::RHICommandBufferPool( RHI::ERHIType::Vulkan )
            {
            }
            VulkanCommandBufferPool( VulkanDevice* pDevice, VulkanCommandQueue* pCommandQueue );
            ~VulkanCommandBufferPool();

            //-------------------------------------------------------------------------

            virtual RHI::RHICommandBuffer* Allocate() override;
            virtual void Restore( RHI::RHICommandBuffer* pCommandBuffer ) override;

            virtual void Reset() override;

            virtual void WaitUntilAllCommandsFinished() override;

            virtual void Submit( RHI::RHICommandBuffer* pCommandBuffer, TSpan<RHI::RHISemaphore*> pWaitSemaphores, TSpan<RHI::RHISemaphore*> pSignalSemaphores, TSpan<Render::PipelineStage> waitStages ) override;

        private:

            void CreateNewPool();

            VulkanCommandBuffer* FindIdleCommandBuffer();

            inline uint32_t GetPoolAllocatedBufferCount() const;

            inline void WaitAllFences( TInlineVector<VkFence, NumMaxCommandBufferPerPool> fences, uint64_t timeout = std::numeric_limits<uint64_t>::max() );

        private:

            VulkanDevice*                           m_pDevice = nullptr;

            TVector<VkCommandPool>                  m_poolHandles;

            AllocatedCommandBufferArray             m_allocatedCommandBuffers;
            TVector<VulkanCommandBuffer*>           m_submittedCommandBuffers;
            TVector<VulkanCommandBuffer*>           m_idleCommandBuffers;

            Threading::Mutex                        m_mutex;
        };
    }
}

#endif