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
        struct VulkanCommonPipelineStates
        {
            VkPipeline                                                              m_pPipeline = nullptr;
            VkPipelineLayout                                                        m_pPipelineLayout = nullptr;

            VkPipelineBindPoint                                                     m_pipelineBindPoint;
        };

        struct VulkanCommonPipelineInfo
        {
            TInlineVector<RHI::SetDescriptorLayout, RHI::NumMaxResourceBindingSet>  m_setDescriptorLayouts;
            TVector<VkDescriptorPoolSize>                                           m_setPoolSizes;
            TInlineVector<VkDescriptorSetLayout, RHI::NumMaxResourceBindingSet>     m_setLayouts;
        };

        class VulkanRasterPipelineState final : public RHI::RHIRasterPipelineState
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanRasterPipelineState()
                : RHIRasterPipelineState( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanRasterPipelineState() = default;

        public:

            inline virtual TInlineVector<RHI::SetDescriptorLayout, RHI::NumMaxResourceBindingSet> const& GetResourceSetLayouts() const { return m_pipelineInfo.m_setDescriptorLayouts; }

        private:

            VulkanCommonPipelineStates                                           m_pipelineState;
            VulkanCommonPipelineInfo                                                m_pipelineInfo;
        };

        class VulkanComputePipelineState final : public RHI::RHIComputePipelineState
        {
            friend class VulkanDevice;
            friend class VulkanCommandBuffer;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanComputePipelineState()
                : RHIComputePipelineState( RHI::ERHIType::Vulkan )
            {
            }
            virtual ~VulkanComputePipelineState() = default;

        public:

            inline virtual TInlineVector<RHI::SetDescriptorLayout, RHI::NumMaxResourceBindingSet> const& GetResourceSetLayouts() const { return m_pipelineInfo.m_setDescriptorLayouts; }

            inline virtual uint32_t GetThreadGroupSizeX() const override { return m_dispathGroupWidth; };
            inline virtual uint32_t GetThreadGroupSizeY() const override { return m_dispathGroupHeight; };
            inline virtual uint32_t GetThreadGroupSizeZ() const override { return m_dispathGroupDepth; };

        private:

            VulkanCommonPipelineStates                                           m_pipelineState;
            VulkanCommonPipelineInfo                                                m_pipelineInfo;

            uint32_t                                                                m_dispathGroupWidth = 1;
            uint32_t                                                                m_dispathGroupHeight = 1;
            uint32_t                                                                m_dispathGroupDepth = 1;
        };
    }
}

#endif