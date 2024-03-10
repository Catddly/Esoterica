#if defined(EE_VULKAN)
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipelineState.h"
#include "VulkanCommandBufferPool.h"
#include "VulkanCommandQueue.h"
#include "RHIToVulkanSpecification.h"
#include "Base/Math/Math.h"
#include "Base/Encoding/Hash.h"
#include "Base/RHI/Resource/RHIDescriptorSet.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        TInlineVector<VkMemoryBarrier, 1> VulkanCommandBuffer::m_sGlobalBarriers;
        TInlineVector<VkBufferMemoryBarrier, 32> VulkanCommandBuffer::m_sBufferBarriers;
        TInlineVector<VkImageMemoryBarrier, 32> VulkanCommandBuffer::m_sTextureBarriers;

        // Synchronization
        //-------------------------------------------------------------------------

        //RHI::RenderCommandSyncPoint VulkanCommandBuffer::SetSyncPoint( Render::PipelineStage stage )
        //{
        //    if ( auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice ) )
        //    {
        //        VkEventCreateInfo createInfo = {};
        //        createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
        //        createInfo.flags = VK_EVENT_CREATE_DEVICE_ONLY_BIT;

        //        VkEvent syncEvent;
        //        VK_SUCCEEDED( vkCreateEvent( pVkDevice->m_pHandle, &createInfo, nullptr, &syncEvent ) );

        //        VkPipelineStageFlags stageFlags = ToVulkanPipelineStageFlags( stage );
        //        m_syncPoints.push_back( { syncEvent, stageFlags } );
        //        vkCmdSetEvent( m_pHandle, syncEvent, stageFlags );

        //        return RHI::RenderCommandSyncPoint{ static_cast<int32_t>( m_syncPoints.size() - 1 ) };
        //    }

        //    return RHI::RenderCommandSyncPoint{ -1 };
        //}

        //void VulkanCommandBuffer::WaitSyncPoint( RHI::RenderCommandSyncPoint syncPoint, Render::PipelineStage stage )
        //{
        //    auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice );
        //    if ( pVkDevice
        //         && syncPoint.m_syncPointIndex > 0 
        //         && syncPoint.m_syncPointIndex < m_syncPoints.size() )
        //    {
        //        auto& srcSyncPoint = m_syncPoints[syncPoint.m_syncPointIndex];

        //        vkCmdWaitEvents( m_pHandle, 1, &srcSyncPoint.first,
        //                         srcSyncPoint.second, ToVulkanPipelineStageFlags( stage ),
        //                         0, nullptr,
        //                         0, nullptr,
        //                         0, nullptr);
        //    }
        //}

        // Render Commands
        //-------------------------------------------------------------------------

        void VulkanCommandBuffer::Draw( uint32_t vertexCount, uint32_t instanceCount /*= 1*/, uint32_t firstIndex /*= 0*/, uint32_t firstInstance /*= 0 */ )
        {
            vkCmdDraw( m_pHandle, vertexCount, instanceCount, firstIndex, firstInstance );
        }

        void VulkanCommandBuffer::DrawIndexed( uint32_t indexCount, uint32_t instanceCount /*= 1*/, uint32_t firstIndex /*= 0*/, int32_t vertexOffset /*= 0*/, uint32_t firstInstance /*= 0*/ )
        {
            vkCmdDrawIndexed( m_pHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
        }

        // Compute Commands
        //-------------------------------------------------------------------------

        void VulkanCommandBuffer::Dispatch( uint32_t groupX, uint32_t groupY, uint32_t groupZ )
        {
            vkCmdDispatch( m_pHandle, groupX, groupY, groupZ );
        }

        // Pipeline Barrier
        //-------------------------------------------------------------------------

        bool VulkanCommandBuffer::BeginRenderPass( RHI::RHIRenderPass* pRenderPass, RHI::RHIFramebuffer* pFramebuffer, RHI::RenderArea const& renderArea, TSpan<RHI::RHITextureView const> textureViews )
        {
            EE_ASSERT( pFramebuffer );
            EE_ASSERT( renderArea.IsValid() );
            EE_ASSERT( !textureViews.empty() );

            if ( pRenderPass )
            {
                if ( auto* pVkRenderPass = RHI::RHIDowncast<VulkanRenderPass>( pRenderPass ) )
                {
                    TFixedVector<VkImageView, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> views;
                    views.reserve( textureViews.size() );
                    for ( RHI::RHITextureView const& texView : textureViews )
                    {
                        views.push_back( reinterpret_cast<VkImageView>( texView.m_pViewHandle ) );
                    }

                    TFixedVector<VkClearValue, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> clearValues;
                    for ( auto const& color : pVkRenderPass->m_desc.m_colorAttachments )
                    {
                        if ( color.m_loadOp == RHI::ERenderPassAttachmentLoadOp::Clear )
                        {
                            Float4 const colorClear = color.m_clearColorValue.ToFloat4();

                            auto& clearValue = clearValues.emplace_back();
                            clearValue.color.float32[0] = colorClear[0];
                            clearValue.color.float32[1] = colorClear[1];
                            clearValue.color.float32[2] = colorClear[2];
                            clearValue.color.float32[3] = colorClear[3];
                        }
                    }

                    if ( pVkRenderPass->m_desc.m_depthAttachment.has_value() && pVkRenderPass->m_desc.m_depthAttachment->m_loadOp == RHI::ERenderPassAttachmentLoadOp::Clear )
                    {
                        auto& clearValue = clearValues.emplace_back();

                        clearValue.depthStencil.depth = pVkRenderPass->m_desc.m_depthAttachment->m_clearDepthValue;
                        clearValue.depthStencil.stencil = pVkRenderPass->m_desc.m_depthAttachment->m_clearStencilValue;
                    }

                    VkRenderPassAttachmentBeginInfo attachmentBeginInfo = {};
                    attachmentBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
                    attachmentBeginInfo.attachmentCount = static_cast<uint32_t>( views.size() );
                    attachmentBeginInfo.pAttachments = views.data();

                    VkRenderPassBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    beginInfo.pNext = &attachmentBeginInfo;
                    beginInfo.renderPass = pVkRenderPass->m_pHandle;
                    // Safety: here we make a copy of a VkFramebuffer pointer, so it is safe to write this without having the risk of dangling reference.
                    beginInfo.framebuffer = RHI::RHIDowncast<VulkanFramebuffer>( pFramebuffer )->m_pHandle;
                    beginInfo.clearValueCount = attachmentBeginInfo.attachmentCount;
                    beginInfo.pClearValues = clearValues.data();
                    beginInfo.renderArea.extent.width = renderArea.m_width;
                    beginInfo.renderArea.extent.height = renderArea.m_height;
                    beginInfo.renderArea.offset.x = renderArea.m_OffsetX;
                    beginInfo.renderArea.offset.y = renderArea.m_OffsetY;

                    vkCmdBeginRenderPass( m_pHandle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );

                    return true;
                }
            }

            EE_LOG_WARNING( "Render", "VulkanCommandBuffer", "Pass in null render pass, reject to begin render pass!" );
            return false;
        }

        bool VulkanCommandBuffer::BeginRenderPassWithClearValue( RHI::RHIRenderPass* pRenderPass, RHI::RHIFramebuffer* pFramebuffer, RHI::RenderArea const& renderArea, TSpan<RHI::RHITextureView const> textureViews, RHI::RenderPassClearValue const& clearValue )
        {
            EE_ASSERT( pFramebuffer );
            EE_ASSERT( renderArea.IsValid() );
            EE_ASSERT( !textureViews.empty() );

            if ( pRenderPass )
            {
                if ( auto* pVkRenderPass = RHI::RHIDowncast<VulkanRenderPass>( pRenderPass ) )
                {
                    Float4 const clearColor = clearValue.m_clearColor.ToFloat4();
                    VkClearValue vkClearValue;
                    vkClearValue.color.float32[0] = clearColor[0];
                    vkClearValue.color.float32[1] = clearColor[1];
                    vkClearValue.color.float32[2] = clearColor[2];
                    vkClearValue.color.float32[3] = clearColor[3];

                    vkClearValue.depthStencil.depth = clearValue.m_depth;
                    vkClearValue.depthStencil.stencil = clearValue.m_stencil;

                    TFixedVector<VkImageView, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> views;
                    views.reserve( textureViews.size() );
                    for ( RHI::RHITextureView const& texView : textureViews )
                    {
                        views.push_back( reinterpret_cast<VkImageView>( texView.m_pViewHandle ) );
                    }

                    VkRenderPassAttachmentBeginInfo attachmentBeginInfo = {};
                    attachmentBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
                    attachmentBeginInfo.attachmentCount = static_cast<uint32_t>( views.size() );
                    attachmentBeginInfo.pAttachments = views.data();

                    VkRenderPassBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    beginInfo.pNext = &attachmentBeginInfo;
                    beginInfo.renderPass = pVkRenderPass->m_pHandle;
                    // Safety: here we make a copy of a VkFramebuffer pointer, so it is safe to write this without having the risk of dangling reference.
                    beginInfo.framebuffer = RHI::RHIDowncast<VulkanFramebuffer>( pFramebuffer )->m_pHandle;
                    beginInfo.clearValueCount = 1;
                    beginInfo.pClearValues = &vkClearValue;
                    beginInfo.renderArea.extent.width = renderArea.m_width;
                    beginInfo.renderArea.extent.height = renderArea.m_height;
                    beginInfo.renderArea.offset.x = renderArea.m_OffsetX;
                    beginInfo.renderArea.offset.y = renderArea.m_OffsetY;

                    vkCmdBeginRenderPass( m_pHandle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );

                    return true;
                }
            }

            EE_LOG_WARNING( "Render", "VulkanCommandBuffer", "Pass in null render pass, reject to begin render pass!" );
            return false;
        }

        void VulkanCommandBuffer::EndRenderPass()
        {
            vkCmdEndRenderPass( m_pHandle );
        }

        void VulkanCommandBuffer::PipelineBarrier(
            RHI::GlobalBarrier const* pGlobalBarriers,
            uint32_t bufferBarrierCount, RHI::BufferBarrier const* pBufferBarriers,
            uint32_t textureBarrierCount, RHI::TextureBarrier const* pTextureBarriers 
        )
        {
            VkPipelineStageFlags srcStageFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStageFlag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

            if ( pGlobalBarriers != nullptr )
            {
                auto transition = GetMemoryBarrierTransition( *pGlobalBarriers );
                srcStageFlag |= transition.m_srcStage;
                dstStageFlag |= transition.m_dstStage;
                m_sGlobalBarriers.push_back( transition.m_barrier );
            }

            for ( uint32_t i = 0; i < bufferBarrierCount; ++i )
            {
                auto const& bufferBarrier = pBufferBarriers[i];
                auto transition = GetBufferBarrierTransition( bufferBarrier );
                srcStageFlag |= transition.m_srcStage;
                dstStageFlag |= transition.m_dstStage;
                m_sBufferBarriers.push_back( transition.m_barrier );
            }

            for ( uint32_t i = 0; i < textureBarrierCount; ++i )
            {
                auto const& textureBarrier = pTextureBarriers[i];
                auto transition = GetTextureBarrierTransition( textureBarrier );
                srcStageFlag |= transition.m_srcStage;
                dstStageFlag |= transition.m_dstStage;
                m_sTextureBarriers.push_back( transition.m_barrier );
            }

            vkCmdPipelineBarrier(
                m_pHandle,
                srcStageFlag, dstStageFlag,
                0,
                static_cast<uint32_t>( m_sGlobalBarriers.size() ), m_sGlobalBarriers.data(),
                static_cast<uint32_t>( m_sBufferBarriers.size() ), m_sBufferBarriers.data(),
                static_cast<uint32_t>( m_sTextureBarriers.size() ), m_sTextureBarriers.data()
            );

            m_sGlobalBarriers.clear();
            m_sBufferBarriers.clear();
            m_sTextureBarriers.clear();
        }
    
        // Resource Binding
        //-------------------------------------------------------------------------

        void VulkanCommandBuffer::BindPipelineState( RHI::RHIPipelineState* pRhiPipelineState )
        {
            if ( pRhiPipelineState )
            {
                RHI::RHIPipelineType const pipelineType = pRhiPipelineState->GetPipelineType();

                VulkanCommonPipelineStates const* pVkPipelineState = nullptr;
                if ( pipelineType == RHI::RHIPipelineType::Raster )
                {
                    auto* pVkRasterPipelineState = RHI::RHIDowncast<VulkanRasterPipelineState>( pRhiPipelineState );
                    EE_ASSERT( pVkRasterPipelineState );
                    pVkPipelineState = &pVkRasterPipelineState->m_pipelineState;
                }
                else if ( pipelineType == RHI::RHIPipelineType::Compute )
                {
                    auto* pVkComputePipelineState = RHI::RHIDowncast<VulkanComputePipelineState>( pRhiPipelineState );
                    EE_ASSERT( pVkComputePipelineState );
                    pVkPipelineState = &pVkComputePipelineState->m_pipelineState;
                }
                EE_ASSERT( pVkPipelineState );

                vkCmdBindPipeline( m_pHandle, pVkPipelineState->m_pipelineBindPoint, pVkPipelineState->m_pPipeline );
            }
            else
            {
                EE_LOG_WARNING( "Render", "VulkanCommandBuffer", "Pass in null pipeline state, reject to bind pipeline state!" );
            }
        }

        void VulkanCommandBuffer::BindDescriptorSetInPlace( uint32_t set, RHI::RHIPipelineState const* pPipelineState, TSpan<RHI::RHIPipelineBinding const> const& bindings )
        {
            static VkDescriptorSet lastBoundVkDescriptorSet = nullptr;

            VulkanDescriptorSetHash const hash = { set, bindings };

            EE_ASSERT( pPipelineState );
            
            RHI::RHIPipelineType const pipelineType = pPipelineState->GetPipelineType();

            VulkanCommonPipelineInfo const* pVkPipelineInfo = nullptr;
            VulkanCommonPipelineStates const* pVkPipelineState = nullptr;
            if ( pipelineType == RHI::RHIPipelineType::Raster )
            {
                auto* pVkRasterPipelineState = RHI::RHIDowncast<VulkanRasterPipelineState>( pPipelineState );
                EE_ASSERT( pVkRasterPipelineState );
                pVkPipelineInfo = &pVkRasterPipelineState->m_pipelineInfo;
                pVkPipelineState = &pVkRasterPipelineState->m_pipelineState;
            }
            else if ( pipelineType == RHI::RHIPipelineType::Compute )
            {
                auto* pVkComputePipelineState = RHI::RHIDowncast<VulkanComputePipelineState>( pPipelineState );
                EE_ASSERT( pVkComputePipelineState );
                pVkPipelineInfo = &pVkComputePipelineState->m_pipelineInfo;
                pVkPipelineState = &pVkComputePipelineState->m_pipelineState;
            }
            EE_ASSERT( pVkPipelineInfo );
            EE_ASSERT( pVkPipelineState );

            auto vkSet = CreateOrFindInPlaceUpdatedDescriptorSet( hash, *pVkPipelineInfo );

            if ( !vkSet )
            {
                return;
            }

            if ( lastBoundVkDescriptorSet == vkSet )
            {
                return;
            }
            lastBoundVkDescriptorSet = vkSet;

            EE_ASSERT( m_pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice );
            EE_ASSERT( pVkDevice );

            TInlineVector<uint32_t, 4> dynOffsets;
            size_t const setHashValue = hash.GetHash();
            if ( !IsInPlaceDescriptorSetUpdated( setHashValue ) )
            {
                TSInlineList<VkDescriptorBufferInfo, 8> bufferInfos;
                TSInlineList<VkDescriptorImageInfo, 8> textureInfos;
                auto writes = WriteDescriptorSets( vkSet, pVkPipelineInfo->m_setDescriptorLayouts[set], bindings, bufferInfos, textureInfos, dynOffsets );

                vkUpdateDescriptorSets( pVkDevice->m_pHandle, static_cast<uint32_t>( writes.size() ), writes.data(), 0, nullptr );
                MarkAsUpdated( setHashValue, vkSet );
            }

            vkCmdBindDescriptorSets(
                m_pHandle, pVkPipelineState->m_pipelineBindPoint, pVkPipelineState->m_pPipelineLayout,
                set, 1, &vkSet,
                static_cast<uint32_t>( dynOffsets.size() ), !dynOffsets.empty() ? dynOffsets.data() : nullptr
            );
        }

        void VulkanCommandBuffer::BindVertexBuffer( uint32_t firstBinding, TSpan<RHI::RHIBuffer const*> pVertexBuffers, uint32_t offset )
        {
            VkDeviceSize const bufferOffset = static_cast<VkDeviceSize>( offset );
            uint32_t const bindingCount = static_cast<uint32_t>( pVertexBuffers.size() );

            TInlineVector<VkBuffer, 8> pVkBuffers;
            for ( auto const* pVertexBuffer : pVertexBuffers )
            {
                if ( auto* pVkBuffer = RHI::RHIDowncast<VulkanBuffer>( pVertexBuffer ) )
                {
                    pVkBuffers.push_back( pVkBuffer->m_pHandle );
                }
            }

            if ( !pVkBuffers.empty() )
            {
                vkCmdBindVertexBuffers( m_pHandle, firstBinding, bindingCount, pVkBuffers.data(), &bufferOffset );
            }
        }

        void VulkanCommandBuffer::BindIndexBuffer( RHI::RHIBuffer const* pIndexBuffer, uint32_t offset )
        {
            VkDeviceSize bufferOffset = static_cast<VkDeviceSize>( offset );
            if ( auto* pVkBuffer = RHI::RHIDowncast<VulkanBuffer>( pIndexBuffer ) )
            {
                vkCmdBindIndexBuffer( m_pHandle, pVkBuffer->m_pHandle, bufferOffset, VK_INDEX_TYPE_UINT32 );
            }
        }

        void VulkanCommandBuffer::UpdateDescriptorSetBinding( uint32_t set, uint32_t binding, RHI::RHIPipelineState const* pPipelineState, RHI::RHIPipelineBinding const& rhiBinding )
        {
            EE_ASSERT( pPipelineState );

            RHI::RHIPipelineType const pipelineType = pPipelineState->GetPipelineType();

            VulkanCommonPipelineInfo const* pVkPipelineInfo = nullptr;
            if ( pipelineType == RHI::RHIPipelineType::Raster )
            {
                auto* pVkPipelineState = RHI::RHIDowncast<VulkanRasterPipelineState>( pPipelineState );
                EE_ASSERT( pVkPipelineState );
                pVkPipelineInfo = &pVkPipelineState->m_pipelineInfo;
            }
            else if ( pipelineType == RHI::RHIPipelineType::Compute )
            {
                auto* pVkPipelineState = RHI::RHIDowncast<VulkanComputePipelineState>( pPipelineState );
                EE_ASSERT( pVkPipelineState );
                pVkPipelineInfo = &pVkPipelineState->m_pipelineInfo;
            }
            EE_ASSERT( pVkPipelineInfo );
            
            RHI::RHIPipelineBinding const bindingsRef[] = { rhiBinding };
            VulkanDescriptorSetHash const hash = {
                set,
                bindingsRef
            };

            size_t const setHashValue = hash.GetHash();
            if ( IsInPlaceDescriptorSetUpdated( setHashValue ) ) // Only update once
            {
                return;
            }

            VkDescriptorSet vkSet = CreateOrFindInPlaceUpdatedDescriptorSet( hash, *pVkPipelineInfo );
            if ( !vkSet )
            {
                return;
            }

            EE_ASSERT( m_pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice );
            EE_ASSERT( pVkDevice );

            TSInlineList<VkDescriptorBufferInfo, 8> bufferInfos;
            TSInlineList<VkDescriptorImageInfo, 8> textureInfos;
            TInlineVector<uint32_t, 4> dynOffsets;
            auto write = WriteDescriptorSet(
                vkSet, binding, pVkPipelineInfo->m_setDescriptorLayouts[set], rhiBinding,
                bufferInfos, textureInfos, dynOffsets
            );

            vkUpdateDescriptorSets( pVkDevice->m_pHandle, 1, &write, 0, nullptr );
            MarkAsUpdated( setHashValue, vkSet );
        }

        // State Settings
        //-------------------------------------------------------------------------

        void VulkanCommandBuffer::ClearColor( Color color )
        {
            EE_UNIMPLEMENTED_FUNCTION();
        }

        void VulkanCommandBuffer::ClearDepthStencil( RHI::RHITexture* pTexture, RHI::TextureSubresourceRange range, RHI::ETextureLayout currentLayout, float depthValue, uint32_t stencil )
        {
            if ( auto* pVkTexture = RHI::RHIDowncast<VulkanTexture>( pTexture ) )
            {
                if ( !pVkTexture->GetDesc().m_usage.IsFlagSet( RHI::ETextureUsage::DepthStencil ) )
                {
                    EE_LOG_WARNING( "Render", "Vulkan Command Buffer", "Try to clear a non-depth stencil texture!" );
                    return;
                }

                VkClearDepthStencilValue clearValue;
                clearValue.depth = depthValue;
                clearValue.stencil = stencil;

                VkImageSubresourceRange vkRange;
                vkRange.baseArrayLayer = range.m_baseArrayLayer;
                vkRange.baseMipLevel = range.m_baseMipLevel;
                vkRange.layerCount = range.m_layerCount;
                vkRange.levelCount = range.m_levelCount;

                if ( range.m_aspectFlags.IsAnyFlagSet() )
                {
                    vkRange.aspectMask = ToVulkanImageAspectFlags( range.m_aspectFlags );
                }
                else
                {
                    vkRange.aspectMask = SpeculateImageAspectFlagsFromUsageAndFormat( pTexture->GetDesc().m_usage, pTexture->GetDesc().m_format );
                }

                vkCmdClearDepthStencilImage( 
                    m_pHandle, pVkTexture->m_pHandle, ToVulkanImageLayout( currentLayout ),
                    &clearValue,
                    1u, &vkRange
                );
            }
        }

        void VulkanCommandBuffer::SetViewport( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
        {
            EE_ASSERT( static_cast<uint32_t>( Math::Abs( xOffset ) ) <= width );
            EE_ASSERT( static_cast<uint32_t>( Math::Abs( yOffset ) ) <= height );

            // Note: upside-down vulkan viewport to manually flip vulkan NDC y-axis.
            VkViewport viewport = {};
            viewport.width = static_cast<float>( width );
            viewport.height = -static_cast<float>( height );
            viewport.x = static_cast<float>( xOffset );
            viewport.y = static_cast<float>( height + yOffset );
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport( m_pHandle, 0, 1, &viewport );
        }

        void VulkanCommandBuffer::SetScissor( uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset )
        {
            VkRect2D cullRect = {};
            cullRect.extent.width = width;
            cullRect.extent.height = height;
            cullRect.offset.x = xOffset;
            cullRect.offset.y = yOffset;
            vkCmdSetScissor( m_pHandle, 0, 1, &cullRect );
        }

        // Resource Copying
        //-------------------------------------------------------------------------

        void VulkanCommandBuffer::CopyBufferToBuffer( RHI::RHIBuffer* pSrcBuffer, RHI::RHIBuffer* pDstBuffer )
        {
            EE_ASSERT( pSrcBuffer->GetDesc().m_desireSize == pDstBuffer->GetDesc().m_desireSize );

            auto* pVkSrcBuffer = RHI::RHIDowncast<VulkanBuffer>( pSrcBuffer );
            auto* pVkDstBuffer = RHI::RHIDowncast<VulkanBuffer>( pDstBuffer );
            EE_ASSERT( pVkSrcBuffer );
            EE_ASSERT( pVkDstBuffer );

            RHI::RenderResourceBarrierState prevSrcBarrierState = SpeculateBarrierStateFromUsage( pSrcBuffer->GetDesc().m_usage );
            RHI::RenderResourceBarrierState prevDstBarrierState = SpeculateBarrierStateFromUsage( pDstBuffer->GetDesc().m_usage );
            RHI::RenderResourceBarrierState nextSrcBarrierState = RHI::RenderResourceBarrierState::TransferRead;
            RHI::RenderResourceBarrierState nextDstBarrierState = RHI::RenderResourceBarrierState::TransferWrite;

            RHI::BufferBarrier bufferBarriers[2];
            bufferBarriers[0].m_size = pSrcBuffer->GetDesc().m_desireSize;
            bufferBarriers[0].m_offset = 0;
            bufferBarriers[0].m_pRhiBuffer = pSrcBuffer;
            // No queue resource ownership transfer happened
            bufferBarriers[0].m_srcQueueFamilyIndex = m_pCommandBufferPool->GetQueueIndex();
            bufferBarriers[0].m_dstQueueFamilyIndex = bufferBarriers[0].m_srcQueueFamilyIndex;
            bufferBarriers[0].m_previousAccessesCount = 1;
            bufferBarriers[0].m_pPreviousAccesses = &prevSrcBarrierState;
            bufferBarriers[0].m_nextAccessesCount = 1;
            bufferBarriers[0].m_pNextAccesses = &nextSrcBarrierState;

            bufferBarriers[1].m_size = pDstBuffer->GetDesc().m_desireSize;
            bufferBarriers[1].m_offset = 0;
            bufferBarriers[1].m_pRhiBuffer = pDstBuffer;
            // No queue resource ownership transfer happened
            bufferBarriers[1].m_srcQueueFamilyIndex = m_pCommandBufferPool->GetQueueIndex();
            bufferBarriers[1].m_dstQueueFamilyIndex = bufferBarriers[1].m_srcQueueFamilyIndex;
            bufferBarriers[1].m_previousAccessesCount = 1;
            bufferBarriers[1].m_pPreviousAccesses = &prevDstBarrierState;
            bufferBarriers[1].m_nextAccessesCount = 1;
            bufferBarriers[1].m_pNextAccesses = &nextDstBarrierState;

            PipelineBarrier(
                nullptr,
                2, bufferBarriers,
                0, nullptr
            );

            VkBufferCopy bufferCopy = {};
            // TODO: support partial copy
            bufferCopy.size = bufferBarriers[0].m_size;
            bufferCopy.srcOffset = 0;
            bufferCopy.dstOffset = 0;

            vkCmdCopyBuffer( m_pHandle, pVkSrcBuffer->m_pHandle, pVkDstBuffer->m_pHandle, 1, &bufferCopy );

            nextSrcBarrierState = prevSrcBarrierState;
            nextDstBarrierState = prevDstBarrierState;
            prevSrcBarrierState = RHI::RenderResourceBarrierState::TransferRead;
            prevDstBarrierState = RHI::RenderResourceBarrierState::TransferWrite;

            PipelineBarrier(
                nullptr,
                2, bufferBarriers,
                0, nullptr
            );
        }

        void VulkanCommandBuffer::CopyBufferToTexture( RHI::RHITexture* pDstTexture, RHI::RenderResourceBarrierState dstBarrier, TSpan<RHI::TextureSubresourceRangeUploadRef> const uploadDataRef )
        {
            EE_ASSERT( pDstTexture );

            for ( RHI::TextureSubresourceRangeUploadRef const& subresourceRef : uploadDataRef )
            {
                TInlineVector<VkBufferImageCopy, 12> subresourceLayers;

                EE_ASSERT( subresourceRef.m_pStagingBuffer );

                if ( subresourceRef.m_uploadAllMips )
                {
                    uint32_t const bufferSize = subresourceRef.m_pStagingBuffer->GetDesc().m_desireSize;

                    auto& texDesc = pDstTexture->GetDesc();
                    uint32_t remainingBufferSize = bufferSize;
                    // upload from lowest mip to highest
                    for ( uint32_t mip = 0; mip < texDesc.m_mipmap; ++mip )
                    {
                        uint32_t const width = Math::Max( texDesc.m_width >> mip, 1u );
                        uint32_t const height = Math::Max( texDesc.m_height >> mip, 1u );
                        uint32_t const depth = Math::Max( texDesc.m_depth >> mip, 1u );
                        
                        // Note: not support 3D texture for nows
                        EE_ASSERT( depth == 1 );

                        uint32_t currentMipByteSize = 0;
                        uint32_t currentMipByteSizePerRow = 0;
                        GetPixelFormatByteSize( width, height, texDesc.m_format, currentMipByteSize, currentMipByteSizePerRow );

                        EE_ASSERT( currentMipByteSize != 0 && currentMipByteSizePerRow != 0 );

                        VkDeviceSize currentBufferOffset = 0;
                        if ( remainingBufferSize >= currentMipByteSize )
                        {
                            currentBufferOffset = static_cast<VkDeviceSize>( bufferSize - remainingBufferSize );
                            remainingBufferSize -= currentMipByteSize;
                        }
                        else // uint32_t overflow
                        {
                            // skip upload for this mip
                            EE_LOG_WARNING( "Render", "CopyBufferToTexture", "Inconsist mip chain, failed to upload mip %u", mip );
                            remainingBufferSize = 0;
                            break;
                        }

                        auto& imageCopy = subresourceLayers.emplace_back();

                        imageCopy.imageSubresource.baseArrayLayer = subresourceRef.m_layer;
                        imageCopy.imageSubresource.layerCount = 1;
                        imageCopy.imageSubresource.mipLevel = mip;

                        if ( subresourceRef.m_aspectFlags.IsAnyFlagSet() )
                        {
                            imageCopy.imageSubresource.aspectMask = ToVulkanImageAspectFlags( subresourceRef.m_aspectFlags );
                        }
                        else
                        {
                            imageCopy.imageSubresource.aspectMask = SpeculateImageAspectFlagsFromUsageAndFormat( pDstTexture->GetDesc().m_usage, pDstTexture->GetDesc().m_format );
                        }

                        imageCopy.bufferRowLength = 0;
                        imageCopy.bufferImageHeight = 0;
                        imageCopy.bufferOffset = currentBufferOffset;
                        
                        imageCopy.imageExtent.width = width;
                        imageCopy.imageExtent.height = height;
                        imageCopy.imageExtent.depth = depth;
                        imageCopy.imageOffset.x = 0;
                        imageCopy.imageOffset.y = 0;
                        imageCopy.imageOffset.z = 0;
                    }

                    EE_ASSERT( remainingBufferSize == 0 );
                }
                else
                {
                    auto& texDesc = pDstTexture->GetDesc();
                    auto& imageCopy = subresourceLayers.emplace_back();

                    imageCopy.imageSubresource.baseArrayLayer = subresourceRef.m_layer;
                    imageCopy.imageSubresource.layerCount = 1;
                    imageCopy.imageSubresource.mipLevel = subresourceRef.m_baseMipLevel;

                    if ( subresourceRef.m_aspectFlags.IsAnyFlagSet() )
                    {
                        imageCopy.imageSubresource.aspectMask = ToVulkanImageAspectFlags( subresourceRef.m_aspectFlags );
                    }
                    else
                    {
                        imageCopy.imageSubresource.aspectMask = SpeculateImageAspectFlagsFromUsageAndFormat( pDstTexture->GetDesc().m_usage, pDstTexture->GetDesc().m_format );
                    }

                    imageCopy.bufferRowLength = 0;
                    imageCopy.bufferImageHeight = 0;
                    imageCopy.bufferOffset = 0;

                    imageCopy.imageExtent.width = Math::Max( texDesc.m_width >> imageCopy.imageSubresource.mipLevel, 1u );
                    imageCopy.imageExtent.height = Math::Max( texDesc.m_height >> imageCopy.imageSubresource.mipLevel, 1u );
                    imageCopy.imageExtent.depth = Math::Max( texDesc.m_depth >> imageCopy.imageSubresource.mipLevel, 1u );
                    imageCopy.imageOffset.x = 0;
                    imageCopy.imageOffset.y = 0;
                    imageCopy.imageOffset.z = 0;
                }

                // upload data for each layer
                //-------------------------------------------------------------------------

                RHI::RenderResourceBarrierState prevBarrierState = dstBarrier;
                RHI::RenderResourceBarrierState nextBarrierState = RHI::RenderResourceBarrierState::TransferWrite;

                RHI::TextureBarrier barrier;
                barrier.m_discardContents = subresourceRef.m_layer == 0;
                barrier.m_pRhiTexture = pDstTexture;
                // for now, no queue resource transfer
                barrier.m_srcQueueFamilyIndex = m_pCommandBufferPool->GetQueueIndex();
                barrier.m_dstQueueFamilyIndex = barrier.m_srcQueueFamilyIndex;
                barrier.m_previousLayout = RHI::TextureMemoryLayout::Optimal;
                barrier.m_nextLayout = RHI::TextureMemoryLayout::Optimal;
                barrier.m_previousAccessesCount = 1;
                barrier.m_pPreviousAccesses = &prevBarrierState;
                barrier.m_nextAccessesCount = 1;
                barrier.m_pNextAccesses = &nextBarrierState;
                barrier.m_subresourceRange = RHI::TextureSubresourceRange::AllSubresources(
                    ToEngineTextureAspectFlags( SpeculateImageAspectFlagsFromUsageAndFormat( pDstTexture->GetDesc().m_usage, pDstTexture->GetDesc().m_format ) )
                );

                PipelineBarrier(
                    nullptr,
                    0, nullptr,
                    1, &barrier
                );

                auto* pVkBuffer = RHI::RHIDowncast<VulkanBuffer>( subresourceRef.m_pStagingBuffer );
                EE_ASSERT( pVkBuffer );
                auto* pVkDstTexture = RHI::RHIDowncast<VulkanTexture>( pDstTexture );
                EE_ASSERT( pVkDstTexture );

                vkCmdCopyBufferToImage(
                    m_pHandle,
                    pVkBuffer->m_pHandle, pVkDstTexture->m_pHandle,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    static_cast<uint32_t>( subresourceLayers.size() ), subresourceLayers.data()
                );

                barrier.m_discardContents = false;
                barrier.m_pPreviousAccesses = &nextBarrierState;
                barrier.m_pNextAccesses = &prevBarrierState;

                PipelineBarrier(
                    nullptr,
                    0, nullptr,
                    1, &barrier
                );
            }
        }

        // Vulkan Pipeline Barrier Utility Functions
        //-------------------------------------------------------------------------

        static bool IsWriteAccess( RHI::RenderResourceBarrierState const& access )
        {
            switch ( access )
            {
                case RHI::RenderResourceBarrierState::VertexShaderWrite:
                case RHI::RenderResourceBarrierState::TessellationControlShaderWrite:
                case RHI::RenderResourceBarrierState::TessellationEvaluationShaderWrite:
                case RHI::RenderResourceBarrierState::GeometryShaderWrite:
                case RHI::RenderResourceBarrierState::FragmentShaderWrite:
                case RHI::RenderResourceBarrierState::ColorAttachmentWrite:
                case RHI::RenderResourceBarrierState::DepthStencilAttachmentWrite:
                case RHI::RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly:
                case RHI::RenderResourceBarrierState::StencilAttachmentWriteDepthReadOnly:
                case RHI::RenderResourceBarrierState::ComputeShaderWrite:
                case RHI::RenderResourceBarrierState::AnyShaderWrite:
                case RHI::RenderResourceBarrierState::TransferWrite:
                case RHI::RenderResourceBarrierState::HostWrite:

                case RHI::RenderResourceBarrierState::General:
                case RHI::RenderResourceBarrierState::ColorAttachmentReadWrite:

                return true;
                break;

                default:
                return false;
                break;
            }
        }

        static VkAccessInfo GetAccessInfo( RHI::RenderResourceBarrierState const& barrierState )
        {
            switch ( barrierState )
            {
                case RHI::RenderResourceBarrierState::Undefined:
                return VkAccessInfo{ 0, 0, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::IndirectBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::VertexBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::IndexBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };

                case RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::VertexShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::VertexShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::TessellationControlShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::TessellationControlShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::TessellationEvaluationShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::TessellationEvaluationShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::GeometryShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::GeometryShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::FragmentShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                // QA
                case RHI::RenderResourceBarrierState::FragmentShaderReadColorInputAttachment:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                // QA
                case RHI::RenderResourceBarrierState::FragmentShaderReadDepthStencilInputAttachment:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::FragmentShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::ColorAttachmentRead:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case RHI::RenderResourceBarrierState::DepthStencilAttachmentRead:
                return VkAccessInfo{ VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

                case RHI::RenderResourceBarrierState::ComputeShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::ComputeShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::ComputeShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::AnyShaderReadUniformBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_UNIFORM_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::AnyShaderReadUniformBufferOrVertexBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::AnyShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::AnyShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::TransferRead:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
                case RHI::RenderResourceBarrierState::HostRead:
                return VkAccessInfo{ VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::Present:
                return VkAccessInfo{ 0, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

                case RHI::RenderResourceBarrierState::VertexShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::TessellationControlShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::TessellationEvaluationShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::GeometryShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::FragmentShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::ColorAttachmentWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case RHI::RenderResourceBarrierState::DepthStencilAttachmentWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                case RHI::RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly:
                return VkAccessInfo{ VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::StencilAttachmentWriteDepthReadOnly:
                return VkAccessInfo{ VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL };

                case RHI::RenderResourceBarrierState::ComputeShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::AnyShaderWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };
                case RHI::RenderResourceBarrierState::TransferWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
                case RHI::RenderResourceBarrierState::HostWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::ColorAttachmentReadWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                case RHI::RenderResourceBarrierState::General:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::RayTracingShaderReadSampledImageOrUniformTexelBuffer:
                return VkAccessInfo{ VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::RayTracingShaderReadColorInputAttachment:
                return VkAccessInfo{ VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::RayTracingShaderReadDepthStencilInputAttachment:
                return VkAccessInfo{ VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                case RHI::RenderResourceBarrierState::RayTracingShaderReadAccelerationStructure:
                return VkAccessInfo{ VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::RayTracingShaderReadOther:
                return VkAccessInfo{ VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL };

                case RHI::RenderResourceBarrierState::AccelerationStructureBuildWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::AccelerationStructureBuildRead:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, VK_IMAGE_LAYOUT_UNDEFINED };
                case RHI::RenderResourceBarrierState::AccelerationStructureBufferWrite:
                return VkAccessInfo{ VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED };

                default:
                EE_UNREACHABLE_CODE();
                return VkAccessInfo{};
            }
        }

        VkMemoryBarrierTransition VulkanCommandBuffer::GetMemoryBarrierTransition( RHI::GlobalBarrier const& globalBarrier )
        {
            VkMemoryBarrierTransition barrier = {};
            barrier.m_srcStage = VkFlags( 0 );
            barrier.m_dstStage = VkFlags( 0 );
            barrier.m_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.m_barrier.pNext = nullptr;
            barrier.m_barrier.srcAccessMask = VkFlags( 0 );
            barrier.m_barrier.dstAccessMask = VkFlags( 0 );

            for ( uint32_t i = 0; i < globalBarrier.m_previousAccessesCount; ++i )
            {
                auto const& prevAccess = globalBarrier.m_pPreviousAccesses[i];
                auto accessInfo = GetAccessInfo( prevAccess );

                // what stage this resource is used in previous stage
                barrier.m_srcStage |= accessInfo.m_stageMask;

                // only access the write access
                if ( IsWriteAccess( prevAccess ) )
                {
                    barrier.m_barrier.srcAccessMask |= accessInfo.m_accessMask;
                }
            }

            for ( uint32_t i = 0; i < globalBarrier.m_nextAccessesCount; ++i )
            {
                auto const& nextAccess = globalBarrier.m_pNextAccesses[i];
                auto accessInfo = GetAccessInfo( nextAccess );

                // what stage this resource is used in previous stage
                barrier.m_dstStage |= accessInfo.m_stageMask;

                // if write access happend before, it must be visible to the dst access.
                // (i.e. RAW (Read-After-Write) operation or WAW)
                if ( barrier.m_barrier.srcAccessMask != VkFlags( 0 ) )
                {
                    barrier.m_barrier.dstAccessMask |= accessInfo.m_accessMask;
                }
            }

            // ensure that the stage masks are valid if no stages were determined
            if ( barrier.m_srcStage == VkFlags( 0 ) )
            {
                barrier.m_srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }

            if ( barrier.m_dstStage == VkFlags( 0 ) )
            {
                barrier.m_dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            return barrier;
        }

        VkBufferBarrierTransition VulkanCommandBuffer::GetBufferBarrierTransition( RHI::BufferBarrier const& bufferBarrier )
        {
            auto* pVkBuffer = RHI::RHIDowncast<VulkanBuffer>( bufferBarrier.m_pRhiBuffer );
            EE_ASSERT( pVkBuffer != nullptr );

            VkBufferBarrierTransition barrier = {};
            barrier.m_srcStage = VkFlags( 0 );
            barrier.m_dstStage = VkFlags( 0 );
            barrier.m_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.m_barrier.pNext = nullptr;
            barrier.m_barrier.srcAccessMask = VkFlags( 0 );
            barrier.m_barrier.dstAccessMask = VkFlags( 0 );
            barrier.m_barrier.srcQueueFamilyIndex = bufferBarrier.m_srcQueueFamilyIndex;
            barrier.m_barrier.dstQueueFamilyIndex = bufferBarrier.m_dstQueueFamilyIndex;
            barrier.m_barrier.buffer = pVkBuffer->m_pHandle;
            barrier.m_barrier.offset = static_cast<VkDeviceSize>( bufferBarrier.m_offset );
            barrier.m_barrier.size = static_cast<VkDeviceSize>( bufferBarrier.m_size );

            for ( uint32_t i = 0; i < bufferBarrier.m_previousAccessesCount; ++i )
            {
                auto const& prevAccess = bufferBarrier.m_pPreviousAccesses[i];
                auto accessInfo = GetAccessInfo( prevAccess );

                // what stage this resource is used in previous stage
                barrier.m_srcStage |= accessInfo.m_stageMask;

                // only access the write access
                if ( IsWriteAccess( prevAccess ) )
                {
                    barrier.m_barrier.srcAccessMask |= accessInfo.m_accessMask;
                }
            }

            for ( uint32_t i = 0; i < bufferBarrier.m_nextAccessesCount; ++i )
            {
                auto const& nextAccess = bufferBarrier.m_pNextAccesses[i];
                auto accessInfo = GetAccessInfo( nextAccess );

                // what stage this resource is used in previous stage
                barrier.m_dstStage |= accessInfo.m_stageMask;

                // if write access happend before, it must be visible to the dst access.
                // (i.e. RAW (Read-After-Write) operation or WAW)
                if ( barrier.m_barrier.srcAccessMask != VkFlags( 0 ) )
                {
                    barrier.m_barrier.dstAccessMask |= accessInfo.m_accessMask;
                }
            }

            // ensure that the stage masks are valid if no stages were determined
            if ( barrier.m_srcStage == VkFlags( 0 ) )
            {
                barrier.m_srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }

            if ( barrier.m_dstStage == VkFlags( 0 ) )
            {
                barrier.m_dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            return barrier;
        }

        VkTextureBarrierTransition VulkanCommandBuffer::GetTextureBarrierTransition( RHI::TextureBarrier const& textureBarrier )
        {
            auto* pVkTexture = RHI::RHIDowncast<VulkanTexture>( textureBarrier.m_pRhiTexture );
            EE_ASSERT( pVkTexture != nullptr );

            VkTextureBarrierTransition barrier = {};
            barrier.m_srcStage = VkFlags( 0 );
            barrier.m_dstStage = VkFlags( 0 );
            barrier.m_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.m_barrier.pNext = nullptr;
            barrier.m_barrier.srcAccessMask = VkFlags( 0 );
            barrier.m_barrier.dstAccessMask = VkFlags( 0 );
            barrier.m_barrier.srcQueueFamilyIndex = textureBarrier.m_srcQueueFamilyIndex;
            barrier.m_barrier.dstQueueFamilyIndex = textureBarrier.m_dstQueueFamilyIndex;
            barrier.m_barrier.image = pVkTexture->m_pHandle;

            barrier.m_barrier.subresourceRange.aspectMask = ToVulkanImageAspectFlags( textureBarrier.m_subresourceRange.m_aspectFlags );
            barrier.m_barrier.subresourceRange.baseMipLevel = textureBarrier.m_subresourceRange.m_baseMipLevel;
            barrier.m_barrier.subresourceRange.levelCount = textureBarrier.m_subresourceRange.m_levelCount;
            barrier.m_barrier.subresourceRange.baseArrayLayer = textureBarrier.m_subresourceRange.m_baseArrayLayer;
            barrier.m_barrier.subresourceRange.layerCount = textureBarrier.m_subresourceRange.m_layerCount;

            for ( uint32_t i = 0; i < textureBarrier.m_previousAccessesCount; ++i )
            {
                auto const& prevAccess = textureBarrier.m_pPreviousAccesses[i];
                auto accessInfo = GetAccessInfo( prevAccess );

                // what stage this resource is used in previous stage
                barrier.m_srcStage |= accessInfo.m_stageMask;

                // only access the write access
                if ( IsWriteAccess( prevAccess ) )
                {
                    barrier.m_barrier.srcAccessMask |= accessInfo.m_accessMask;
                }

                if ( textureBarrier.m_discardContents )
                {
                    // we don't care about the previous image layout
                    barrier.m_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
                else
                {
                    switch ( textureBarrier.m_previousLayout )
                    {
                        case RHI::TextureMemoryLayout::Optimal:
                        barrier.m_barrier.newLayout = accessInfo.m_imageLayout;
                        break;
                        case RHI::TextureMemoryLayout::General:
                        {
                            if ( prevAccess == RHI::RenderResourceBarrierState::Present )
                            {
                                barrier.m_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                            }
                            else
                            {
                                barrier.m_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                            }
                        }
                        break;
                        case RHI::TextureMemoryLayout::GeneralAndPresentation:
                        EE_UNIMPLEMENTED_FUNCTION();
                    }
                }
            }

            for ( uint32_t i = 0; i < textureBarrier.m_nextAccessesCount; ++i )
            {
                auto const& nextAccess = textureBarrier.m_pNextAccesses[i];
                auto accessInfo = GetAccessInfo( nextAccess );

                // what stage this resource is used in previous stage
                barrier.m_dstStage |= accessInfo.m_stageMask;

                // if write access happend before, it must be visible to the dst access.
                // (i.e. RAW (Read-After-Write) operation or WAW)
                if ( barrier.m_barrier.srcAccessMask != VkFlags( 0 ) )
                {
                    barrier.m_barrier.dstAccessMask |= accessInfo.m_accessMask;
                }

                switch ( textureBarrier.m_nextLayout )
                {
                    case RHI::TextureMemoryLayout::Optimal:
                    barrier.m_barrier.newLayout = accessInfo.m_imageLayout;
                    break;
                    case RHI::TextureMemoryLayout::General:
                    {
                        if ( nextAccess == RHI::RenderResourceBarrierState::Present )
                        {
                            barrier.m_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        }
                        else
                        {
                            barrier.m_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        }
                    }
                    break;
                    case RHI::TextureMemoryLayout::GeneralAndPresentation:
                    EE_UNIMPLEMENTED_FUNCTION();
                }
            }

            // ensure that the stage masks are valid if no stages were determined
            if ( barrier.m_srcStage == VkFlags( 0 ) )
            {
                barrier.m_srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }

            if ( barrier.m_dstStage == VkFlags( 0 ) )
            {
                barrier.m_dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }

            return barrier;
        }

        // Vulkan descriptor binding helper functions
        //-------------------------------------------------------------------------

        VkWriteDescriptorSet VulkanCommandBuffer::WriteDescriptorSet(
            VkDescriptorSet set, uint32_t binding, RHI::SetDescriptorLayout const& setDescriptorLayout, RHI::RHIPipelineBinding const& rhiBinding,
            TSInlineList<VkDescriptorBufferInfo, 8>& bufferInfos, TSInlineList<VkDescriptorImageInfo, 8>& textureInfos, TInlineVector<uint32_t, 4> dynOffsets )
        {
            auto iterator = setDescriptorLayout.find( binding );
            if ( iterator == setDescriptorLayout.end() )
            {
                // must match shader descriptor layouts
                return {};
            }

            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = binding;
            write.dstSet = set;
            write.dstArrayElement = 0;

            if ( rhiBinding.m_binding.index() == GetVariantTypeIndex<decltype( rhiBinding.m_binding ), RHI::RHIBufferBinding>() )
            {
                auto const& bufferBinding = eastl::get<RHI::RHIBufferBinding>( rhiBinding.m_binding );

                EE_ASSERT( RHI::RHIDowncast<VulkanBuffer>( bufferBinding.m_pBuffer ) );
                bufferInfos.emplace_front();
                auto& bufferInfo = bufferInfos.front();
                bufferInfo.buffer = RHI::RHIDowncast<VulkanBuffer>( bufferBinding.m_pBuffer )->m_pHandle;
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;

                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.pBufferInfo = &bufferInfos.front();
                write.descriptorCount = 1;
            }
            else if ( rhiBinding.m_binding.index() == GetVariantTypeIndex<decltype( rhiBinding.m_binding ), RHI::RHIDynamicBufferBinding>() )
            {
                auto const& dynBufferBinding = eastl::get<RHI::RHIDynamicBufferBinding>( rhiBinding.m_binding );

                EE_ASSERT( RHI::RHIDowncast<VulkanBuffer>( dynBufferBinding.m_pBuffer ) );
                bufferInfos.emplace_front();
                auto& bufferInfo = bufferInfos.front();
                bufferInfo.buffer = RHI::RHIDowncast<VulkanBuffer>( dynBufferBinding.m_pBuffer )->m_pHandle;
                bufferInfo.offset = dynBufferBinding.m_dynamicOffset;
                bufferInfo.range = VK_WHOLE_SIZE; // TODO: dynamic buffer max uniform buffer range

                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                write.pBufferInfo = &bufferInfos.front();
                write.descriptorCount = 1;

                dynOffsets.push_back( static_cast<uint32_t>( bufferInfo.offset ) );
            }
            else if ( rhiBinding.m_binding.index() == GetVariantTypeIndex<decltype( rhiBinding.m_binding ), RHI::RHITextureBinding>() )
            {
                auto const& textureBinding = eastl::get<RHI::RHITextureBinding>( rhiBinding.m_binding );

                textureInfos.emplace_front();
                auto& textureInfo = textureInfos.front();
                textureInfo.imageView = reinterpret_cast<VkImageView>( textureBinding.m_view.m_pViewHandle );
                textureInfo.sampler = nullptr;
                textureInfo.imageLayout = ToVulkanImageLayout( textureBinding.m_layout );

                if ( textureInfo.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
                {
                    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                }
                else if ( textureInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL )
                {
                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                }
                else
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                }
                write.pImageInfo = &textureInfos.front();
                write.descriptorCount = 1;
            }
            else if ( rhiBinding.m_binding.index() == GetVariantTypeIndex<decltype( rhiBinding.m_binding ), RHI::RHITextureArrayBinding>() )
            {
                auto const& textureArrayBinding = eastl::get<RHI::RHITextureArrayBinding>( rhiBinding.m_binding );
                EE_ASSERT( !textureArrayBinding.m_bindings.empty() );

                for ( auto const& texBind : textureArrayBinding.m_bindings )
                {
                    textureInfos.emplace_front();
                    auto& textureInfo = textureInfos.front();
                    // just take the first one and assume all texture should have the layout
                    textureInfo.imageLayout = ToVulkanImageLayout( textureArrayBinding.m_bindings[0].m_layout );
                    textureInfo.sampler = nullptr;

                    textureInfo.imageView = reinterpret_cast<VkImageView>( textureArrayBinding.m_bindings[0].m_view.m_pViewHandle );

                    if ( textureInfo.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
                    {
                        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    }
                    else if ( textureInfo.imageLayout == VK_IMAGE_LAYOUT_GENERAL )
                    {
                        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    }
                    else
                    {
                        EE_UNIMPLEMENTED_FUNCTION();
                    }
                    write.pImageInfo = &textureInfos.front();
                    write.descriptorCount = 1;
                }
            }
            else if ( rhiBinding.m_binding.index() == GetVariantTypeIndex<decltype( rhiBinding.m_binding ), RHI::RHIUnknownBinding>() )
            {
                EE_LOG_WARNING( "RHI", "Command Buffer", "Found unknown binding while binding pipline descriptor set!" );
            }

            return write;
        }

        TVector<VkWriteDescriptorSet> VulkanCommandBuffer::WriteDescriptorSets(
            VkDescriptorSet set, RHI::SetDescriptorLayout const& setDescriptorLayout, TSpan<RHI::RHIPipelineBinding const> const& bindings,
            TSInlineList<VkDescriptorBufferInfo, 8>& bufferInfos, TSInlineList<VkDescriptorImageInfo, 8>& textureInfos, TInlineVector<uint32_t, 4> dynOffsets
        )
        {
            TVector<VkWriteDescriptorSet> writes;

            uint32_t bindingIndex = 0;
            uint32_t currentBinding = 0;
            for ( auto const& [ _unused, bindingType ] : setDescriptorLayout )
            {
                // skip static sampler binding
                if ( bindingType != RHI::EBindingResourceType::Sampler )
                {
                    // skip static sampler binding
                    VkWriteDescriptorSet write = WriteDescriptorSet(
                        set, bindingIndex, setDescriptorLayout, bindings[currentBinding],
                        bufferInfos, textureInfos, dynOffsets
                    );

                    if ( write.descriptorCount > 0 )
                    {
                        writes.push_back( write );
                    }

                    ++currentBinding;
                }

                ++bindingIndex;
            }

            return writes;
        }

        void VulkanCommandBuffer::MarkAsUpdated( size_t setHashValue, VkDescriptorSet vkSet )
        {
            m_updatedDescriptorSets.insert_or_assign( setHashValue, vkSet);
        }

        bool VulkanCommandBuffer::IsInPlaceDescriptorSetUpdated( size_t setHashValue )
        {
            if ( m_updatedDescriptorSets.find( setHashValue ) == m_updatedDescriptorSets.end() )
            {
                return false;
            }
            return true;
        }

        VkDescriptorSet VulkanCommandBuffer::CreateOrFindInPlaceUpdatedDescriptorSet( VulkanDescriptorSetHash const& hash, VulkanCommonPipelineInfo const& vkPipelineInfo )
        {
            size_t const hashValue = hash.GetHash();
            auto iterator = m_updatedDescriptorSets.find( hashValue );
            if ( iterator != m_updatedDescriptorSets.end() )
            {
                return iterator->second;
            }

            EE_ASSERT( m_pDevice );
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice );
            EE_ASSERT( pVkDevice );

            if ( vkPipelineInfo.m_setDescriptorLayouts.empty() || vkPipelineInfo.m_setDescriptorLayouts.size() <= hash.m_set )
            {
                return nullptr;
            }

            VkDescriptorPoolCreateInfo poolCI = {};
            poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolCI.maxSets = 1;
            poolCI.poolSizeCount = 1;
            poolCI.pPoolSizes = vkPipelineInfo.m_setPoolSizes[hash.m_set].data();
            // TODO: pool update after bind

            VkDescriptorPool vkPool;
            VK_SUCCEEDED( vkCreateDescriptorPool( pVkDevice->m_pHandle, &poolCI, nullptr, &vkPool ) );

            RHI::RHIDescriptorPool deferReleasePool;
            deferReleasePool.m_pSetPoolHandle = vkPool;
            deferReleasePool.SetRHIReleaseImpl( &Vulkan::gDescriptorSetReleaseImpl );

            m_pDevice->DeferRelease( deferReleasePool );

            VkDescriptorSetAllocateInfo setAllocateInfo = {};
            setAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            setAllocateInfo.descriptorSetCount = 1;
            setAllocateInfo.pSetLayouts = &vkPipelineInfo.m_setLayouts[hash.m_set];
            setAllocateInfo.descriptorPool = vkPool;

            VkDescriptorSet vkSet = nullptr;
            VK_SUCCEEDED( vkAllocateDescriptorSets( pVkDevice->m_pHandle, &setAllocateInfo, &vkSet ) );
            EE_ASSERT( vkSet );

            return vkSet;
        }

        //-------------------------------------------------------------------------

		void VulkanCommandBuffer::CleanUp()
		{
            m_updatedDescriptorSets.clear();

            if ( auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( m_pDevice ) )
            {
                for ( auto& syncEvent : m_syncPoints )
                {
                    vkDestroyEvent( pVkDevice->m_pHandle, syncEvent.first, nullptr );
                }
            }
		}

        //-------------------------------------------------------------------------

        size_t VulkanDescriptorSetHash::GetHash() const
        {
            size_t hash = 0;
            Hash::HashCombine( hash, m_set );
            for ( RHI::RHIPipelineBinding const& binding : m_bindings )
            {
                Hash::HashCombine( hash, binding.GetHash() );
            }
            return hash;
        }
    }
}

#endif