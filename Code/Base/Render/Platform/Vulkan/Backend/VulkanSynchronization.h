#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RHI { class RHIDevice; }

namespace EE::Render
{
    namespace Backend
    {
        struct VulkanCPUGPUSyncImpl final : public RHI::IRHICPUGPUSyncImpl
        {
            ~VulkanCPUGPUSyncImpl() = default;

            virtual void* Create( RHI::RHIDevice* pDevice, bool bSignaled = false ) override;
            virtual void  Destroy( RHI::RHIDevice* pDevice, void*& pCPUGPUSync ) override;

            virtual void Reset( RHI::RHIDevice* pDevice, void* pCPUGPUSync ) override;
            virtual void WaitFor( RHI::RHIDevice* pDevice, void* pCPUGPUSync, uint64_t waitTime = std::numeric_limits<uint64_t>::max() ) override;
        };
    }
}

#endif