#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Threading/Threading.h"
#include "Base/RHI/RHIObject.h"
#include "Base/RHI/RHICommandBufferPool.h"

#include <vulkan/vulkan_core.h>
#include <limits>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanCommandBufferPool final : public RHI::RHICommandBufferPool
        {
            EE_RHI_OBJECT( Vulkan, RHICommandBufferPool )

            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

            constexpr static uint32_t NumMaxCommandBufferPerPool = 32;

            using AllocatedCommandBufferArray = TVector<TFixedVector<RHI::RHICommandBufferRef, NumMaxCommandBufferPerPool>>;

        public:

            VulkanCommandBufferPool()
                : RHI::RHICommandBufferPool( RHI::ERHIType::Vulkan )
            {
            }
            VulkanCommandBufferPool( RHI::RHIDeviceRef& pDevice, RHI::RHICommandQueueRef& pCommandQueue );
            ~VulkanCommandBufferPool();

            //-------------------------------------------------------------------------

            virtual RHI::RHICommandBufferRef Allocate() override;
            virtual void Restore( RHI::RHICommandBufferRef& pCommandBuffer ) override;

            virtual void Reset() override;

            virtual void WaitUntilAllCommandsFinished() override;

            virtual void Submit( RHI::RHICommandBufferRef& pCommandBuffer, TSpan<RHI::RHISemaphoreRef&> pWaitSemaphores, TSpan<RHI::RHISemaphoreRef&> pSignalSemaphores, TSpan<Render::PipelineStage> waitStages ) override;

        private:

            void CreateNewPool();

            RHI::RHICommandBufferRef FindIdleCommandBuffer();

            inline uint32_t GetPoolAllocatedBufferCount() const;

            inline void WaitAllFences( TInlineVector<VkFence, NumMaxCommandBufferPerPool> fences, uint64_t timeout = std::numeric_limits<uint64_t>::max() );

        private:

            RHI::RHIDeviceRef                       m_pDevice;

            TVector<VkCommandPool>                  m_poolHandles;

            AllocatedCommandBufferArray             m_allocatedCommandBuffers;
            TVector<RHI::RHICommandBufferRef>       m_submittedCommandBuffers;
            TVector<RHI::RHICommandBufferRef>       m_idleCommandBuffers;

            Threading::Mutex                        m_mutex;
        };
    }
}

#endif