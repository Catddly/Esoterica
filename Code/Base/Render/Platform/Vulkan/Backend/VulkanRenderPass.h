#pragma once
#if defined(EE_VULKAN)

#include "Base/RHI/RHIObject.h"
#include "Base/RHI/Resource/RHIRenderPass.h"

#include <vulkan/vulkan_core.h>

namespace EE::RHI
{
    class RHIDevice;
}

namespace EE::Render
{
    namespace Backend
    {
        class VulkanFramebuffer : public RHI::RHIFramebuffer
        {
            EE_RHI_OBJECT( Vulkan, RHIFramebuffer )

            friend class VulkanDevice;
            friend class VulkanCommandBuffer;
            friend class VulkanFramebufferCache;

        public:

            VulkanFramebuffer()
                : RHIFramebuffer( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanFramebuffer() = default;

        private:

            VkFramebuffer                       m_pHandle = nullptr;
        };

        class VulkanFramebufferCache final : public RHI::RHIFramebufferCache
        {
        private:

            virtual RHI::RHIFramebuffer* CreateFramebuffer( RHI::RHIDevice* pDevice, RHI::RHIFramebufferCacheKey const& key ) override;
            virtual void                 DestroyFramebuffer( RHI::RHIDevice* pDevice, RHI::RHIFramebuffer* pFramebuffer ) override;
        };

        class VulkanRenderPass : public RHI::RHIRenderPass
        {
            EE_RHI_OBJECT( Vulkan, RHIRenderPass )

            friend class VulkanDevice;
            friend class VulkanCommandBuffer;
            friend class VulkanFramebufferCache;

        public:

            VulkanRenderPass();
            virtual ~VulkanRenderPass();
        
        private:

            VkRenderPass                        m_pHandle = nullptr;
        };
    }
}

#endif