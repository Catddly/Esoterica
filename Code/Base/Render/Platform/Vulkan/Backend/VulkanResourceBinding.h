#pragma once

#include "Base/RHI/Resource/RHIResourceBinding.h"

namespace EE::Render
{
    namespace Backend
    {
        class VulkanResourceBinding final : public RHI::RHIResourceBinding
        {
            friend class VulkanDevice;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanResourceBinding()
                : RHIResourceBinding( RHI::ERHIType::Vulkan )
            {
            }
            VulkanResourceBinding( RHI::RHIResourceView* pView )
                : RHIResourceBinding( RHI::ERHIType::Vulkan )
            {
                m_pOwnedView = pView;
            }
            virtual ~VulkanResourceBinding() = default;

        private:
            
            
        };
    }
}