#if defined(EE_VULKAN)
#include "VulkanCommandBufferPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandQueue.h"
#include "VulkanSemaphore.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "RHIToVulkanSpecification.h"
#include "Base/Threading/Threading.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        VulkanCommandBuffer* VulkanCommandBufferPool::Allocate()
        {
            if ( !m_pDevice || !m_pCommandQueue )
            {
                return nullptr;
            }
            
            // Try find available command buffer in pool
            //-------------------------------------------------------------------------

            auto* pCommandBuffer = FindIdleCommandBuffer();
            if ( pCommandBuffer )
            {
                return pCommandBuffer;
            }

            // No idle command buffer, create a new one
            //-------------------------------------------------------------------------

            auto pool = m_poolHandles.back();

            auto* pVkCommandBuffer = EE::New<VulkanCommandBuffer>();
            if ( pVkCommandBuffer )
            {
                EE_ASSERT( m_poolHandles.size() == m_allocatedCommandBuffers.size() );

                VkCommandBufferAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandPool = pool;
                allocInfo.commandBufferCount = 1;

                if ( m_allocatedCommandBuffers.size() < NumMaxCommandBufferPerPool ) // pool still have space
                {
                    VkCommandBuffer cmdBuffer;
                    VK_SUCCEEDED( vkAllocateCommandBuffers( m_pDevice->m_pHandle, &allocInfo, &cmdBuffer ) );

                    pVkCommandBuffer->m_pHandle = cmdBuffer;
                    auto& allocatedCommandBufferArray = m_allocatedCommandBuffers.back();
                    allocatedCommandBufferArray.push_back( pVkCommandBuffer );;
                }
                else // pool exhausted
                {
                    CreateNewPool();

                    pool = m_poolHandles.back();

                    VkCommandBuffer cmdBuffer;
                    VK_SUCCEEDED( vkAllocateCommandBuffers( m_pDevice->m_pHandle, &allocInfo, &cmdBuffer ) );

                    pVkCommandBuffer->m_pHandle = cmdBuffer;
                    auto& newCommandBufferArray = m_allocatedCommandBuffers.push_back();
                    newCommandBufferArray.push_back( pVkCommandBuffer );

                    EE_ASSERT( m_poolHandles.size() == m_allocatedCommandBuffers.size() );
                }

                pVkCommandBuffer->m_pCommandBufferPool = this;
                pVkCommandBuffer->m_pDevice = m_pDevice;
                pVkCommandBuffer->m_sync = RHI::RHICPUGPUSync::Create( m_pDevice, &Vulkan::gCPUGPUSyncImpl, true );
                pVkCommandBuffer->m_pInnerPoolIndex = static_cast<uint32_t>( m_poolHandles.size() - 1 );

                return pVkCommandBuffer;
            }

            return nullptr;
        }

        void VulkanCommandBufferPool::SubmitToQueue( VulkanCommandBuffer* pCommandBuffer, TSpan<RHI::RHISemaphore*> pWaitSemaphores, TSpan<RHI::RHISemaphore*> pSignalSemaphores, TSpan<Render::PipelineStage> waitStages )
        {
            EE_ASSERT( waitStages.size() == pWaitSemaphores.size() );

            TInlineVector<VkSemaphore, 8> waitSemaphores;
            TInlineVector<VkSemaphore, 8> signalSemaphores;
            TInlineVector<VkPipelineStageFlags, 8> waitDstStages;

            waitSemaphores.reserve( pWaitSemaphores.size() );
            signalSemaphores.reserve( pSignalSemaphores.size() );
            waitDstStages.reserve( waitStages.size() );

            if ( pCommandBuffer )
            {
                EE_ASSERT( !pCommandBuffer->IsRecording() );

                auto* pCommandBufferPool = pCommandBuffer->m_pCommandBufferPool;
                EE_ASSERT( pCommandBufferPool );
                auto* pCommandQueue = pCommandBufferPool->m_pCommandQueue;
                EE_ASSERT( pCommandQueue );

                if ( pCommandBuffer->m_sync.IsValid() )
                {
                    pCommandBuffer->m_sync.Reset( m_pDevice );
                }

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.pCommandBuffers = &pCommandBuffer->m_pHandle;
                submitInfo.commandBufferCount = 1;

                if ( !pWaitSemaphores.empty() )
                {
                    for ( auto& pWaitSemaphore : pWaitSemaphores )
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

                if ( !pSignalSemaphores.empty() )
                {
                    for ( auto& pSignalSemaphore : pSignalSemaphores )
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

                if ( !waitStages.empty() )
                {
                    for ( auto& waitStage : waitStages )
                    {
                        waitDstStages.push_back( ToVulkanPipelineStageFlags( waitStage ) );
                    }

                    submitInfo.pWaitDstStageMask = waitDstStages.data();
                }
                else
                {
                    submitInfo.pWaitDstStageMask = nullptr;
                }

                VkFence fence = pCommandBuffer->m_sync.IsValid() ? reinterpret_cast<VkFence>( pCommandBuffer->m_sync.m_pHandle ) : nullptr;
                
                // Note: [Temporary] Use lock to let different threads can submit to the same queue.
                pCommandQueue->Lock();
                VK_SUCCEEDED( vkQueueSubmit( pCommandQueue->m_pHandle, 1, &submitInfo, fence ) );
                pCommandQueue->Unlock();

                m_submittedCommandBuffers.push_back( pCommandBuffer );
            }
        }

        void VulkanCommandBufferPool::Reset()
        {
            for ( auto& pool : m_poolHandles )
            {
                VK_SUCCEEDED( vkResetCommandPool( m_pDevice->m_pHandle, pool, 0 ) );
            }

            m_idleCommandBuffers.clear();
            for ( auto const& commandBuffers : m_allocatedCommandBuffers )
            {
                for ( auto& pCommandBuffer : commandBuffers )
                {
                    // command buffer pool is reset, clean up old states in command buffer
                    pCommandBuffer->CleanUp();
                    m_idleCommandBuffers.push_back( pCommandBuffer );
                }
            }

            m_submittedCommandBuffers.clear();
            m_bReadyToAllocate = true;
        }

        void VulkanCommandBufferPool::WaitUntilAllCommandsFinish()
        {
            TInlineVector<VkFence, NumMaxCommandBufferPerPool> toWaitFences;
            for ( auto* pSubmittedCommandBuffer : m_submittedCommandBuffers )
            {
                if ( pSubmittedCommandBuffer->m_sync.IsValid() )
                {
                    toWaitFences.push_back( reinterpret_cast<VkFence>( pSubmittedCommandBuffer->m_sync.m_pHandle ) );
                }
            }

            if ( !toWaitFences.empty() )
            {
                WaitAllFences( toWaitFences );
            }

            m_bReadyToAllocate = false;
        }

        //-------------------------------------------------------------------------

        VulkanCommandBufferPool::VulkanCommandBufferPool( VulkanDevice* pDevice, VulkanCommandQueue* pCommandQueue )
            : m_pDevice( pDevice ), m_pCommandQueue( pCommandQueue )
        {
            EE_ASSERT( m_pDevice );
            EE_ASSERT( m_pCommandQueue );

            // TODO: assertion on constructor
            CreateNewPool();
            EE_ASSERT( m_poolHandles.size() == 1 );
        }

        VulkanCommandBufferPool::~VulkanCommandBufferPool()
        {
            m_pDevice->WaitUntilIdle();

            for ( auto& commandBuffers : m_allocatedCommandBuffers )
            {
                for ( auto& commandBuffer : commandBuffers )
                {
                    RHI::RHICPUGPUSync::Destroy( m_pDevice, commandBuffer->m_sync );
                    EE::Delete( commandBuffer );
                }
            }

            for ( auto& pool : m_poolHandles )
            {
                vkDestroyCommandPool( m_pDevice->m_pHandle, pool, nullptr );
            }

            m_pCommandQueue = nullptr;
            m_pDevice = nullptr;
        }

        //-------------------------------------------------------------------------

        void VulkanCommandBufferPool::CreateNewPool()
        {
            VkCommandPoolCreateInfo poolCI = {};
            poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.queueFamilyIndex = m_pCommandQueue->GetDeviceIndex();
            poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            VkCommandPool pool;
            VK_SUCCEEDED( vkCreateCommandPool( m_pDevice->m_pHandle, &poolCI, nullptr, &pool ) );
        
            m_poolHandles.push_back( pool );
            m_allocatedCommandBuffers.push_back();
            m_allocatedCommandBuffers.back().reserve( NumMaxCommandBufferPerPool );
        }

        VulkanCommandBuffer* VulkanCommandBufferPool::FindIdleCommandBuffer()
        {
            if ( m_idleCommandBuffers.empty() )
            {
                return nullptr;
            }

            auto* pCommandBuffer = m_idleCommandBuffers.back();
            m_idleCommandBuffers.pop_back();
            return pCommandBuffer;
        }

        uint32_t VulkanCommandBufferPool::GetPoolAllocatedBufferCount() const
        {
            return static_cast<uint32_t>( m_allocatedCommandBuffers.back().size() );
        }

        void VulkanCommandBufferPool::WaitAllFences( TInlineVector<VkFence, NumMaxCommandBufferPerPool> fences, uint64_t timeout )
        {
            VK_SUCCEEDED( vkWaitForFences( m_pDevice->m_pHandle, static_cast<uint32_t>( fences.size() ), fences.data(), true, timeout ) );
        }
    }
}

#endif