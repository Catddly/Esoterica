#if defined(EE_VULKAN)
#include "VulkanRenderPass.h"
#include "VulkanDevice.h"
#include "Base/Types/Arrays.h"
#include "Base/RHI/RHIDowncastHelper.h"
#include "VulkanCommon.h"
#include "RHIToVulkanSpecification.h"

namespace EE::Render
{
    namespace Backend
    {
        RHI::RHIFramebuffer* VulkanFramebufferCache::CreateFramebuffer( RHI::RHIDevice* pDevice, RHI::RHIFramebufferCacheKey const& key )
        {
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            if ( !pVkDevice )
            {
                return nullptr;
            }

            auto* pVkRenderPass = RHI::RHIDowncast<VulkanRenderPass>( m_pRenderPass );
            if ( !pVkRenderPass )
            {
                return nullptr;
            }

            TFixedVector<VkFramebufferAttachmentImageInfo, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> attachmentImageInfos;
            TFixedVector<VkFormat, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> attachmentImageFormats;

            for ( size_t i = 0; i < m_attachmentDescs.size(); ++i )
            {
                auto const& hash = key.m_attachmentHashs[i];
                attachmentImageFormats.push_back( ToVulkanFormat( m_attachmentDescs[i].m_pixelFormat ) );

                VkFramebufferAttachmentImageInfo attachmentImageInfo = {};
                attachmentImageInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
                attachmentImageInfo.width = key.m_extentX;
                attachmentImageInfo.height = key.m_extentY;
                attachmentImageInfo.usage = hash.m_usage;
                attachmentImageInfo.flags = hash.m_createFlags;
                attachmentImageInfo.layerCount = 1;
                attachmentImageInfo.viewFormatCount = 1;
                attachmentImageInfo.pViewFormats = &attachmentImageFormats[i];
                attachmentImageInfos.push_back( attachmentImageInfo );
            }

            VkFramebufferAttachmentsCreateInfo attachmentsCI = {};
            attachmentsCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
            attachmentsCI.attachmentImageInfoCount = static_cast<uint32_t>( attachmentImageInfos.size() );
            attachmentsCI.pAttachmentImageInfos = attachmentImageInfos.data();

            VkFramebufferCreateInfo framebufferCI = {};
            framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCI.pNext = &attachmentsCI;
            framebufferCI.width = key.m_extentX;
            framebufferCI.height = key.m_extentY;
            framebufferCI.renderPass = pVkRenderPass->m_pHandle;
            framebufferCI.layers = 1;
            // Note: lazy bound image views
            framebufferCI.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
            framebufferCI.attachmentCount = static_cast<uint32_t>( attachmentImageInfos.size() );
            // Note: we specified VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT flag, so this can be nullptr
            framebufferCI.pAttachments = nullptr;

            auto* pFramebuffer = EE::New<VulkanFramebuffer>();
            if ( !pFramebuffer )
            {
                EE_ASSERT( false );
            }
            VK_SUCCEEDED( vkCreateFramebuffer( pVkDevice->m_pHandle, &framebufferCI, nullptr, &(pFramebuffer->m_pHandle) ) );

            return pFramebuffer;
        }

        void VulkanFramebufferCache::DestroyFramebuffer( RHI::RHIDevice* pDevice, RHI::RHIFramebuffer* pFramebuffer )
        {
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            if ( !pVkDevice )
            {
                EE_ASSERT( false );
                return;
            }

            auto* pVkFramebuffer = RHI::RHIDowncast<VulkanFramebuffer>( pFramebuffer );
            if ( !pVkFramebuffer )
            {
                EE_ASSERT( false );
                return;
            }

            vkDestroyFramebuffer( pVkDevice->m_pHandle, pVkFramebuffer->m_pHandle, nullptr );

            EE::Delete( pFramebuffer );
        }

        //-------------------------------------------------------------------------

        VulkanRenderPass::VulkanRenderPass()
            : RHIRenderPass( RHI::ERHIType::Vulkan )
        {
            m_pFramebufferCache = EE::New<VulkanFramebufferCache>();
            EE_ASSERT( m_pFramebufferCache );
        }

        VulkanRenderPass::~VulkanRenderPass()
        {
            if ( m_pFramebufferCache )
            {
                EE::Delete( m_pFramebufferCache );
            }
        }
    }
}

#endif