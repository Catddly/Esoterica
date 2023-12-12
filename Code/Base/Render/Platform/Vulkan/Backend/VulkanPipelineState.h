#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Map.h"
#include "Base/RHI/Resource/RHIPipelineState.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
    namespace Backend
    {
        class VulkanPipelineState : public RHI::RHIRasterPipelineState
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanPipelineState()
                : RHIRasterPipelineState( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanPipelineState() = default;

        public:

            inline virtual TInlineVector<SetDescriptorLayout, RHI::NumMaxResourceBindingSet> const& GetResourceSetLayouts() const { return m_setDescriptorLayouts; }

        private:

            VkPipeline                                                              m_pPipeline = nullptr;
            VkPipelineLayout                                                        m_pPipelineLayout = nullptr;

            VkPipelineBindPoint                                                     m_pipelineBindPoint;

            TInlineVector<SetDescriptorLayout, RHI::NumMaxResourceBindingSet>       m_setDescriptorLayouts;
            TVector<VkDescriptorPoolSize>                                           m_setPoolSizes;
            TInlineVector<VkDescriptorSetLayout, RHI::NumMaxResourceBindingSet>     m_setLayouts;
        };
    }
}

#endif