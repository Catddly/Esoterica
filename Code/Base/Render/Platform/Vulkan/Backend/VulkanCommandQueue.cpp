#if defined(EE_VULKAN)
#include "VulkanCommandQueue.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "RHIToVulkanSpecification.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        VulkanCommandQueue::VulkanCommandQueue( VulkanDevice* pDevice, RHI::CommandQueueType type, QueueFamily const& queueFamily, uint32_t queueIndex )
            : RHI::RHICommandQueue( RHI::ERHIType::Vulkan ), m_pDevice( pDevice ), m_queueFamily( queueFamily ), m_queueIndex( queueIndex )
        {
            EE_ASSERT( queueIndex < queueFamily.m_props.queueCount );

            vkGetDeviceQueue( m_pDevice->m_pHandle, queueFamily.m_index, queueIndex, &m_pHandle );

            if ( type == RHI::CommandQueueType::Graphic && queueFamily.IsGraphicQueue() )
            {
                m_type = RHI::CommandQueueType::Graphic;
            }
            else if ( type == RHI::CommandQueueType::Compute && queueFamily.IsComputeQueue() )
            {
                m_type = RHI::CommandQueueType::Compute;
            }
            else if ( type == RHI::CommandQueueType::Transfer && queueFamily.IsTransferQueue() )
            {
                m_type = RHI::CommandQueueType::Transfer;
            }

            EE_ASSERT( m_pHandle != nullptr );
        }

        void VulkanCommandQueue::WaitUntilIdle() const
        {
            vkQueueWaitIdle( m_pHandle );
        }

        //void VulkanCommandQueue::AddTransferCommandSyncPoint()
        //{
        //    Threading::ScopeLock lock( m_submitMutex );

        //    if ( !m_waitToSubmitTasks.empty() )
        //    {
        //        auto& lastTask = m_waitToSubmitTasks.back();

        //        m_pDevice->BeginCommandBuffer( lastTask.m_pCommandBuffer );
        //        //lastTask.m_pCommandBuffer->
        //                        

        //        m_pDevice->EndCommandBuffer( lastTask.m_pCommandBuffer );
        //    }
        //}

        void VulkanCommandQueue::FlushToGPU()
		{
            EE_ASSERT( Threading::GetCurrentThreadID() == m_threadId );
            EE_ASSERT( m_pDevice );

            for ( auto& task : m_waitToSubmitTasks )
            {
                TInlineVector<VkSemaphore, 8> waitSemaphores;
                TInlineVector<VkSemaphore, 8> signalSemaphores;
                TInlineVector<VkPipelineStageFlags, 8> waitDstStages;

                waitSemaphores.reserve( task.m_pWaitSemaphores.size() );
                signalSemaphores.reserve( task.m_pSignalSemaphores.size() );
                waitDstStages.reserve( task.m_waitStages.size() );

                if ( auto* pVkCommandBuffer = RHI::RHIDowncast<VulkanCommandBuffer>( task.m_pCommandBuffer ) )
                {
                    EE_ASSERT( !pVkCommandBuffer->IsRecording() );

                    auto* pCommandBufferPool = pVkCommandBuffer->m_pCommandBufferPool;

                    if ( pVkCommandBuffer->m_sync.IsValid() )
                    {
                        pVkCommandBuffer->m_sync.Reset( m_pDevice );
                    }

                    VkSubmitInfo submitInfo = {};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.pCommandBuffers = &pVkCommandBuffer->m_pHandle;
                    submitInfo.commandBufferCount = 1;

                    if ( !task.m_pWaitSemaphores.empty() )
                    {
                        for ( auto& pWaitSemaphore : task.m_pWaitSemaphores )
                        {
                            if ( auto* pVkSemaphore = RHI::RHIDowncast<VulkanSemaphore>( pWaitSemaphore ) )
                            {
                                waitSemaphores.push_back( pVkSemaphore->m_pHandle );
                            }
                        }

                        submitInfo.waitSemaphoreCount = static_cast<uint32_t>( waitSemaphores.size() );
                        submitInfo.pWaitSemaphores = waitSemaphores.data();
                    }
                    else
                    {
                        submitInfo.waitSemaphoreCount = 0;
                        submitInfo.pWaitSemaphores = nullptr;
                    }

                    if ( !task.m_pSignalSemaphores.empty() )
                    {
                        for ( auto& pSignalSemaphore : task.m_pSignalSemaphores )
                        {
                            if ( auto* pVkSemaphore = RHI::RHIDowncast<VulkanSemaphore>( pSignalSemaphore ) )
                            {
                                signalSemaphores.push_back( pVkSemaphore->m_pHandle );
                            }
                        }

                        submitInfo.signalSemaphoreCount = static_cast<uint32_t>( signalSemaphores.size() );
                        submitInfo.pSignalSemaphores = signalSemaphores.data();
                    }
                    else
                    {
                        submitInfo.signalSemaphoreCount = 0;
                        submitInfo.pSignalSemaphores = nullptr;
                    }

                    if ( !task.m_waitStages.empty() )
                    {
                        for ( auto& waitStage : task.m_waitStages )
                        {
                            waitDstStages.push_back( ToVulkanPipelineStageFlags( waitStage ) );
                        }

                        submitInfo.pWaitDstStageMask = waitDstStages.data();
                    }
                    else
                    {
                        submitInfo.pWaitDstStageMask = nullptr;
                    }

                    VkFence fence = task.m_pCommandBuffer->m_sync.IsValid() ? reinterpret_cast<VkFence>( task.m_pCommandBuffer->m_sync.m_pHandle ) : nullptr;
                    VK_SUCCEEDED( vkQueueSubmit( m_pHandle, 1, &submitInfo, fence ) );
                }
            }
		
            m_waitToSubmitTasks.clear();
        }
    }
}

#endif