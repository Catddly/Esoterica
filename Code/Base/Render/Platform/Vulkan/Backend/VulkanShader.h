#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/Resource/RHIShader.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanShader final : public RHI::RHIShader
        {
        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanShader()
                : RHIShader( RHI::ERHIType::Vulkan )
            {}
            ~VulkanShader() = default;

            virtual bool IsValid() const override
            {
                return m_pModule != nullptr;
            }

        private:

            friend class VulkanDevice;

            VkShaderModule              m_pModule = nullptr;
        };
    }
}

#endif // EE_VULKAN