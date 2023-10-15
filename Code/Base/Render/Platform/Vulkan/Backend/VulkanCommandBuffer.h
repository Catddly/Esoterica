#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/RHI/RHICommandBuffer.h"

#include <vulkan/vulkan_core.h>

namespace EE::RHI
{
    class RHIRenderPass;
    class RHIFramebuffer;
    class RHIPipelineState;

    class RHITextureView;
}

namespace EE::Render
{
	namespace Backend
	{
        // Vulkan Pipeline Barrier Utility Types
        //-------------------------------------------------------------------------

        struct VkAccessInfo
        {
            /// Describes which stage in the pipeline this resource is used.
            VkPipelineStageFlags			m_stageMask;
            /// Describes which access mode in the pipeline this resource is used.
            VkAccessFlags					m_accessMask;
            /// Describes the image memory layout which image will be used if this resource is a image resource.
            VkImageLayout					m_imageLayout;
        };

        struct VkMemoryBarrierTransition
        {
            VkPipelineStageFlags				m_srcStage;
            VkPipelineStageFlags				m_dstStage;
            VkMemoryBarrier						m_barrier;
        };

        struct VkBufferBarrierTransition
        {
            VkPipelineStageFlags				m_srcStage;
            VkPipelineStageFlags				m_dstStage;
            VkBufferMemoryBarrier				m_barrier;
        };

        struct VkTextureBarrierTransition
        {
            VkPipelineStageFlags				m_srcStage;
            VkPipelineStageFlags				m_dstStage;
            VkImageMemoryBarrier				m_barrier;
        };

        //-------------------------------------------------------------------------

		class VulkanCommandBuffer : public RHI::RHICommandBuffer
		{
            friend class VulkanDevice;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanCommandBuffer()
                : RHICommandBuffer( RHI::ERHIType::Vulkan )
            {}
            virtual ~VulkanCommandBuffer() = default;

        public:

            // Render Commands
            //-------------------------------------------------------------------------

            virtual void Draw( uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0 ) override;
            virtual void DrawIndexed( uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0 ) override;

            // Pipeline Barrier
            //-------------------------------------------------------------------------

            virtual bool BeginRenderPass( RHI::RHIRenderPass* pRhiRenderPass, RHI::RHIFramebuffer* pFramebuffer, RHI::RenderArea const& renderArea, TSpan<RHI::RHITextureView*> textureViews ) override;
            virtual void EndRenderPass() override;

            virtual void PipelineBarrier( 
                RHI::GlobalBarrier const* pGlobalBarriers,
                uint32_t bufferBarrierCount, RHI::BufferBarrier const* pBufferBarriers,
                uint32_t textureBarrierCount, RHI::TextureBarrier const* pTextureBarriers
            );

            // Resource Binding
            //-------------------------------------------------------------------------

            virtual void BindPipelineState( RHI::RHIPipelineState* pRhiPipelineState ) override;

            // State Settings
            //-------------------------------------------------------------------------

            virtual void SetViewport( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) override;
            virtual void SetScissor( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) override;

			inline VkCommandBuffer Raw() const { return m_pHandle; }

        private:

            // Vulkan Pipeline Barrier Utility Functions
            //-------------------------------------------------------------------------
            
            VkMemoryBarrierTransition GetMemoryBarrierTransition( RHI::GlobalBarrier const& globalBarrier );
            VkBufferBarrierTransition GetBufferBarrierTransition( RHI::BufferBarrier const& bufferBarrier );
            VkTextureBarrierTransition GetTextureBarrierTransition( RHI::TextureBarrier const& textureBarrier );

		private:

            static TInlineVector<VkMemoryBarrier, 1>                    m_sGlobalBarriers;
            static TInlineVector<VkBufferMemoryBarrier, 32>             m_sBufferBarriers;
            static TInlineVector<VkImageMemoryBarrier, 32>              m_sTextureBarriers;
			
            VkCommandBuffer					                            m_pHandle = nullptr;
		};
    }
}

#endif