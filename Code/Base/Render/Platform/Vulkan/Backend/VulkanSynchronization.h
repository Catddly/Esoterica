#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::Render
{
    namespace Backend
    {
        struct VulkanCPUGPUSyncImpl final : public RHI::IRHICPUGPUSyncImpl
        {
            ~VulkanCPUGPUSyncImpl() = default;

            virtual void* Create( RHI::RHIDeviceRef& pDevice, bool bSignaled = false ) override;
            virtual void  Destroy( RHI::RHIDeviceRef& pDevice, void*& pCPUGPUSync ) override;

            virtual void Reset( RHI::RHIDeviceRef& pDevice, void* pCPUGPUSync ) override;
            virtual void WaitFor( RHI::RHIDeviceRef& pDevice, void* pCPUGPUSync, uint64_t waitTime = std::numeric_limits<uint64_t>::max() ) override;
        };
    }
}

#endif