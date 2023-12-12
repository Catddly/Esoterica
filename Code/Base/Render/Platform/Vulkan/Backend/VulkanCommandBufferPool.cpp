#if defined(EE_VULKAN)
#include "VulkanCommandBufferPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandQueue.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

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
                if ( m_allocatedCommandBuffers.size() < NumMaxCommandBufferPerPool )
                {
                    VkCommandBufferAllocateInfo allocInfo = {};
                    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    allocInfo.commandPool = pool;
                    allocInfo.commandBufferCount = NumMaxCommandBufferPerPool;

                    m_allocatedCommandBuffers.back().push_back( pVkCommandBuffer );

                    VK_SUCCEEDED( vkAllocateCommandBuffers( m_pDevice->m_pHandle, &allocInfo, &(pVkCommandBuffer->m_pHandle) ) );
                }
                else // pool exhausted
                {
                    CreateNewPool();

                    pool = m_poolHandles.back();

                    VkCommandBufferAllocateInfo allocInfo = {};
                    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    allocInfo.commandPool = pool;
                    allocInfo.commandBufferCount = NumMaxCommandBufferPerPool;

                    auto& newCommandBufferArray = m_allocatedCommandBuffers.push_back();
                    newCommandBufferArray.push_back( pVkCommandBuffer );

                    VK_SUCCEEDED( vkAllocateCommandBuffers( m_pDevice->m_pHandle, &allocInfo, &(pVkCommandBuffer->m_pHandle) ) );
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

        void VulkanCommandBufferPool::SubmitToQueue( VulkanCommandBuffer* pCommandBuffer )
        {
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
                submitInfo.pWaitSemaphores = nullptr;
                submitInfo.waitSemaphoreCount = 0;
                submitInfo.pSignalSemaphores = nullptr;
                submitInfo.signalSemaphoreCount = 0;
                submitInfo.pWaitDstStageMask = nullptr;

                VkFence fence = pCommandBuffer->m_sync.IsValid() ? reinterpret_cast<VkFence>( pCommandBuffer->m_sync.m_pHandle ) : nullptr;
                VK_SUCCEEDED( vkQueueSubmit( pCommandQueue->m_pHandle, 1, &submitInfo, fence ) );

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
                    m_idleCommandBuffers.push_back( pCommandBuffer );
                }
            }

            m_submittedCommandBuffers.clear();
        }

        void VulkanCommandBufferPool::WaitUntilAllCommandsFinish()
        {
            TInlineVector<VkFence, 2 * NumMaxCommandBufferPerPool> toWaitFences;
            for ( auto* pSubmittedCommandBuffer : m_submittedCommandBuffers )
            {
                if ( pSubmittedCommandBuffer->m_sync.IsValid() )
                {
                    toWaitFences.push_back( reinterpret_cast<VkFence>( pSubmittedCommandBuffer->m_sync.m_pHandle ) );
                }
            }

            WaitAllFences( toWaitFences );
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

            auto& pool = m_poolHandles.push_back();
            m_allocatedCommandBuffers.push_back();

            VK_SUCCEEDED( vkCreateCommandPool( m_pDevice->m_pHandle, &poolCI, nullptr, &pool ) );
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

        void VulkanCommandBufferPool::WaitAllFences( TInlineVector<VkFence, 16> fences, uint64_t timeout )
        {
            VK_SUCCEEDED( vkWaitForFences( m_pDevice->m_pHandle, static_cast<uint32_t>( fences.size() ), fences.data(), true, timeout ) );
        }
    }
}

#endif