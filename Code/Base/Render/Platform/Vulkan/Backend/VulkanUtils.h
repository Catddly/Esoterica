#pragma once
#if defined(EE_VULKAN)

#include "Base/Math/Math.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::Render
{
	namespace Backend::Util
	{
        // TODO: use some platform api instead of writing a own version here
		Int2 GetCurrentActiveWindowUserExtent();

        inline bool IsUniformBuffer( RHI::RHIBufferCreateDesc const& bufferCreateDesc )
        {
            if ( bufferCreateDesc.m_usage.AreAnyFlagsSet(
                RHI::EBufferUsage::Uniform,
                RHI::EBufferUsage::UniformTexel
                ) )
            {
                return true;
            }
            return false;
        }

        inline bool IsStorageBuffer( RHI::RHIBufferCreateDesc const& bufferCreateDesc )
        {
            if ( bufferCreateDesc.m_usage.AreAnyFlagsSet(
                RHI::EBufferUsage::Storage,
                RHI::EBufferUsage::StorageTexel
                ) )
            {
                return true;
            }
            return false;
        }
	}
}

#endif