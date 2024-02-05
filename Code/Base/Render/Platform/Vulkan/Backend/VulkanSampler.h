#pragma once
#if defined(EE_VULKAN)

#include "Base/Encoding/Hash.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        struct VulkanStaticSamplerDesc
        {
            VkFilter filter;
            VkSamplerMipmapMode mipmap;
            VkSamplerAddressMode address;

            inline friend bool operator==( VulkanStaticSamplerDesc const& lhs, VulkanStaticSamplerDesc const& rhs )
            {
                return lhs.filter == rhs.filter
                    && lhs.mipmap == rhs.mipmap
                    && lhs.address == rhs.address;
            }
        };
    }
}

// Support for THashmap
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::Render::Backend::VulkanStaticSamplerDesc>
    {
        eastl_size_t operator()( EE::Render::Backend::VulkanStaticSamplerDesc const& samplerDesc ) const
        {
            eastl_size_t hash = EE::Hash::GetHash64( static_cast<uint32_t>( samplerDesc.filter ) );
            hash ^= EE::Hash::GetHash64( static_cast<uint32_t>( samplerDesc.mipmap ) );
            hash ^= EE::Hash::GetHash64( static_cast<uint32_t>( samplerDesc.address ) );
            return hash;
        }
    };
}

#endif