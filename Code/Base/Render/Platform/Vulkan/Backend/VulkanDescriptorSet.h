#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/Resource/RHIDescriptorSet.h"

namespace EE::Render
{
    namespace Backend
    {
        struct VulkanDescriptorSetReleaseImpl : public RHI::IRHIDescriptorPoolReleaseImpl
        {
            virtual void Release( RHI::RHIDeviceRef& pDevice, void* pSetPool ) override;
        };
    }
}

#endif