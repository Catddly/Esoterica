#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Types/List.h"
#include "Base/Types/HashMap.h"
#include "Base/RHI/Resource/RHITexture.h"
#include "Base/RHI/Resource/RHIPipelineState.h"
#include "Base/RHI/RHICommandBuffer.h"

#include <vulkan/vulkan_core.h>

namespace EE::RHI
{
    class RHIDevice;
    class RHIRenderPass;
    class RHIFramebuffer;
    class RHIPipelineState;

    class RHITexture;
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

        struct VulkanDescriptorSetHash
        {
            uint32_t                                        m_set;
            TSpan<RHI::RHIPipelineBinding const> const&     m_bindings;

            size_t GetHash() const;

            friend bool operator==( VulkanDescriptorSetHash const& lhs, VulkanDescriptorSetHash const& rhs )
            {
                return eastl::tie( lhs.m_set, lhs.m_bindings )
                    == eastl::tie( rhs.m_set, rhs.m_bindings );
            }
        };

        //-------------------------------------------------------------------------

        class VulkanCommandBufferPool;
        class VulkanPipelineState;

		class VulkanCommandBuffer : public RHI::RHICommandBuffer
		{
            friend class VulkanDevice;
            friend class VulkanCommandBufferPool;

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

            VulkanCommandBuffer()
                : RHICommandBuffer( RHI::ERHIType::Vulkan )
            {
                m_updatedDescriptorSets.clear();
            }
            virtual ~VulkanCommandBuffer() = default;

        public:

            // Render Commands
            //-------------------------------------------------------------------------

            virtual void Draw( uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0 ) override;
            virtual void DrawIndexed( uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0 ) override;

            // Pipeline Barrier
            //-------------------------------------------------------------------------

            virtual bool BeginRenderPass( RHI::RHIRenderPass* pRhiRenderPass, RHI::RHIFramebuffer* pFramebuffer, RHI::RenderArea const& renderArea, TSpan<RHI::RHITextureView const> textureViews ) override;
            virtual void EndRenderPass() override;

            virtual void PipelineBarrier( 
                RHI::GlobalBarrier const* pGlobalBarriers,
                uint32_t bufferBarrierCount, RHI::BufferBarrier const* pBufferBarriers,
                uint32_t textureBarrierCount, RHI::TextureBarrier const* pTextureBarriers
            );

            // Resource Binding
            //-------------------------------------------------------------------------

            virtual void BindPipelineState( RHI::RHIPipelineState* pRhiPipelineState ) override;
            virtual void BindDescriptorSetInPlace( uint32_t set, RHI::RHIPipelineState const* pPipelineState, TSpan<RHI::RHIPipelineBinding const> const& bindings ) override;

            virtual void BindVertexBuffer( uint32_t firstBinding, TSpan<RHI::RHIBuffer*> pVertexBuffers, uint32_t offset = 0 ) override;
            virtual void BindIndexBuffer( RHI::RHIBuffer* pIndexBuffer, uint32_t offset = 0 ) override;

            virtual void UpdateDescriptorSetBinding( uint32_t set, uint32_t binding, RHI::RHIPipelineState const* pPipelineState, RHI::RHIPipelineBinding const& rhiBinding ) override;

            // State Settings
            //-------------------------------------------------------------------------

            virtual void SetViewport( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) override;
            virtual void SetScissor( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) override;

            // Resource Copying
            //-------------------------------------------------------------------------

            virtual void CopyBufferToTexture( RHI::RHITexture* pDstTexture, RHI::RenderResourceBarrierState dstBarrier, TSpan<RHI::TextureSubresourceRangeUploadRef> const uploadDataRef ) override;

            //-------------------------------------------------------------------------

			inline VkCommandBuffer Raw() const { return m_pHandle; }

        private:

            // Vulkan pipeline barrier utility functions
            //-------------------------------------------------------------------------
            
            VkMemoryBarrierTransition GetMemoryBarrierTransition( RHI::GlobalBarrier const& globalBarrier );
            VkBufferBarrierTransition GetBufferBarrierTransition( RHI::BufferBarrier const& bufferBarrier );
            VkTextureBarrierTransition GetTextureBarrierTransition( RHI::TextureBarrier const& textureBarrier );

            // Vulkan descriptor binding helper functions
            //-------------------------------------------------------------------------

            VkWriteDescriptorSet WriteDescriptorSet(
                VkDescriptorSet set, uint32_t binding, RHI::RHIPipelineState::SetDescriptorLayout const& setDescriptorLayout, RHI::RHIPipelineBinding const& rhiBinding,
                TSInlineList<VkDescriptorBufferInfo, 8>& bufferInfos, TSInlineList<VkDescriptorImageInfo, 8>& textureInfos, TInlineVector<uint32_t, 4> dynOffsets
            );

            TVector<VkWriteDescriptorSet> WriteDescriptorSets(
                VkDescriptorSet set, RHI::RHIPipelineState::SetDescriptorLayout const& setDescriptorLayout, TSpan<RHI::RHIPipelineBinding const> const& bindings,
                TSInlineList<VkDescriptorBufferInfo, 8>& bufferInfos, TSInlineList<VkDescriptorImageInfo, 8>& textureInfos, TInlineVector<uint32_t, 4> dynOffsets
            );

            inline void MarkAsUpdated( size_t setHashValue, VkDescriptorSet vkSet );
            inline bool IsInPlaceDescriptorSetUpdated( size_t setHashValue );
            
            VkDescriptorSet CreateOrFindInPlaceUpdatedDescriptorSet( VulkanDescriptorSetHash const& hash, VulkanPipelineState const* pVkPipelineState );

            //-------------------------------------------------------------------------

            // clean all old states and prepare for new command enqueue.
            // Usually called after its command pool is reset.
            void CleanUp();

		private:

            static TInlineVector<VkMemoryBarrier, 1>                    m_sGlobalBarriers;
            static TInlineVector<VkBufferMemoryBarrier, 32>             m_sBufferBarriers;
            static TInlineVector<VkImageMemoryBarrier, 32>              m_sTextureBarriers;
			
            RHI::RHIDevice*                                             m_pDevice = nullptr;

            VkCommandBuffer					                            m_pHandle = nullptr;
            VulkanCommandBufferPool*                                    m_pCommandBufferPool = nullptr;
            uint32_t                                                    m_pInnerPoolIndex = 0;

            // Only safe cached hash here
            THashMap<size_t, VkDescriptorSet>                           m_updatedDescriptorSets;
		};
    }
}

// Hash
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::Render::Backend::VulkanDescriptorSetHash>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::Render::Backend::VulkanDescriptorSetHash const& descriptorSetHash ) const noexcept
        {
            return descriptorSetHash.GetHash();
        }
    };
}

#endif