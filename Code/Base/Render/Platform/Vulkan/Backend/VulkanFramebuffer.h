#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Encoding/Hash.h"
#include "Base/RHi/Resource/RHIResourceCreationCommons.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        // Necessary attachment flags to determine a unique VulkanFrameBufferAttachment.
        struct VulkanFramebufferAttachmentHash
        {
            VkImageCreateFlags              m_imageCreateFlags;
            VkImageUsageFlags               m_imageUsageFlags;

            inline friend bool operator==( VulkanFramebufferAttachmentHash const& lhs, VulkanFramebufferAttachmentHash const& rhs )
            {
                return lhs.m_imageCreateFlags == rhs.m_imageCreateFlags
                    && lhs.m_imageUsageFlags == rhs.m_imageUsageFlags;
            }

            inline friend bool operator!=( VulkanFramebufferAttachmentHash const& lhs, VulkanFramebufferAttachmentHash const& rhs )
            {
                return !operator==( lhs, rhs );
            }
        };

        // Key to determine a unique frame buffer.
        struct VulkanFramebufferCacheKey
        {
            using VulkanFrameBufferAttachmentHashes = TFixedVector<VulkanFramebufferAttachmentHash, RHI::RHIRenderPassCreateDesc::NumMaxColorAttachmentCount>;

            uint32_t                                        m_extentX;
            uint32_t                                        m_extentY;
            VulkanFrameBufferAttachmentHashes               m_attachmentHashs;

            friend bool operator==( VulkanFramebufferCacheKey const& lhs, VulkanFramebufferCacheKey const& rhs )
            {
                bool result = lhs.m_extentX == rhs.m_extentX
                    && lhs.m_extentY == rhs.m_extentY
                    && lhs.m_attachmentHashs.size() == rhs.m_attachmentHashs.size();

                if ( !result )
                {
                    return result;
                }

                for ( uint32_t i = 0; i < lhs.m_attachmentHashs.size(); ++i )
                {
                    if ( lhs.m_attachmentHashs[i] != rhs.m_attachmentHashs[i] )
                    {
                        return false;
                    }
                }

                return true;
            }
        };
    }
}


// Support for Hashmap
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::Render::Backend::VulkanFramebufferAttachmentHash>
    {
        eastl_size_t operator()( EE::Render::Backend::VulkanFramebufferAttachmentHash const& attachmentHash ) const
        {
            eastl_size_t hash = eastl::hash<uint32_t>()( static_cast<uint32_t>( attachmentHash.m_imageCreateFlags ) );
            EE::Hash::HashCombine( hash, static_cast<uint32_t>( attachmentHash.m_imageUsageFlags ) );
            return hash;
        }
    };

    template <>
    struct hash<EE::Render::Backend::VulkanFramebufferCacheKey>
    {
        eastl_size_t operator()( EE::Render::Backend::VulkanFramebufferCacheKey const& key ) const
        {
            eastl_size_t hash = eastl::hash<uint32_t>()( key.m_extentX );
            EE::Hash::HashCombine( hash, key.m_extentY );
            for ( auto const& attachmentHash : key.m_attachmentHashs )
            {
                EE::Hash::HashCombine( hash, attachmentHash );
            }
            return hash;
        }
    };
}

#endif