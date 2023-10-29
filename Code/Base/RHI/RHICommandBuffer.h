#pragma once

#include "RHITaggedType.h"
#include "RHICommandBuffer.h"
#include "Resource/RHIResourceCreationCommons.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/BitFlags.h"
#include "Base/Math/Math.h"
// TODO: remove this
#include "Base/Render/RenderTexture.h"

//-------------------------------------------------------------------------
//	Rather than use sophisticated enum and bit flags inside vulkan or DX12
//	to perform resource barrier transform in order to synchronize. Render
//	graph resource barrier simplify this process and erase some of the invalid 
//	and nonsensical combinations of resource barrier.
// 
//	This idea comes from vk-sync-rs (by Graham Wihlidal) and be slightly modified.
//-------------------------------------------------------------------------

namespace EE::RHI
{
    enum class RenderResourceBarrierState
    {
        /// Undefined resource state, primarily use for initialization.
        Undefined = 0,
        /// Read as an indirect buffer for drawing or dispatch
        IndirectBuffer,
        /// Read as a vertex buffer for drawing
        VertexBuffer,
        /// Read as an index buffer for drawing
        IndexBuffer,

        /// Read as a uniform buffer in a vertex shader
        VertexShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a vertex shader
        VertexShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource in a vertex shader
        VertexShaderReadOther,

        /// Read as a uniform buffer in a tessellation control shader
        TessellationControlShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a tessellation control shader
        TessellationControlShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource in a tessellation control shader
        TessellationControlShaderReadOther,

        /// Read as a uniform buffer in a tessellation evaluation shader
        TessellationEvaluationShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a tessellation evaluation shader
        TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource in a tessellation evaluation shader
        TessellationEvaluationShaderReadOther,

        /// Read as a uniform buffer in a geometry shader
        GeometryShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a geometry shader
        GeometryShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource in a geometry shader
        GeometryShaderReadOther,

        /// Read as a uniform buffer in a fragment shader
        FragmentShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a fragment shader
        FragmentShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as an input attachment with a color format in a fragment shader
        FragmentShaderReadColorInputAttachment,
        /// Read as an input attachment with a depth/stencil format in a fragment shader
        FragmentShaderReadDepthStencilInputAttachment,
        /// Read as any other resource in a fragment shader
        FragmentShaderReadOther,

        /// Read by blending/logic operations or subpass load operations
        ColorAttachmentRead,
        /// Read by depth/stencil tests or subpass load operations
        DepthStencilAttachmentRead,

        /// Read as a uniform buffer in a compute shader
        ComputeShaderReadUniformBuffer,
        /// Read as a sampled image/uniform texel buffer in a compute shader
        ComputeShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource in a compute shader
        ComputeShaderReadOther,

        /// Read as a uniform buffer in any shader
        AnyShaderReadUniformBuffer,
        /// Read as a uniform buffer in any shader, or a vertex buffer
        AnyShaderReadUniformBufferOrVertexBuffer,
        /// Read as a sampled image in any shader
        AnyShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as any other resource (excluding attachments) in any shader
        AnyShaderReadOther,

        /// Read as the source of a transfer operation
        TransferRead,
        /// Read on the host
        HostRead,
        /// Read by the presentation engine (i.e. `vkQueuePresentKHR`)
        Present,

        /// Written as any resource in a vertex shader
        VertexShaderWrite,
        /// Written as any resource in a tessellation control shader
        TessellationControlShaderWrite,
        /// Written as any resource in a tessellation evaluation shader
        TessellationEvaluationShaderWrite,
        /// Written as any resource in a geometry shader
        GeometryShaderWrite,
        /// Written as any resource in a fragment shader
        FragmentShaderWrite,

        /// Written as a color attachment during rendering, or via a subpass store op
        ColorAttachmentWrite,
        /// Written as a depth/stencil attachment during rendering, or via a subpass store op
        DepthStencilAttachmentWrite,
        /// Written as a depth aspect of a depth/stencil attachment during rendering, whilst the
        /// stencil aspect is read-only. Requires `VK_KHR_maintenance2` to be enabled.
        DepthAttachmentWriteStencilReadOnly,
        /// Written as a stencil aspect of a depth/stencil attachment during rendering, whilst the
        /// depth aspect is read-only. Requires `VK_KHR_maintenance2` to be enabled.
        StencilAttachmentWriteDepthReadOnly,

        /// Written as any resource in a compute shader
        ComputeShaderWrite,

        /// Written as any resource in any shader
        AnyShaderWrite,
        /// Written as the destination of a transfer operation
        TransferWrite,
        /// Written on the host
        HostWrite,

        /// Read or written as a color attachment during rendering
        ColorAttachmentReadWrite,
        /// Covers any access - useful for debug, generally avoid for performance reasons
        General,

        /// Read as a sampled image/uniform texel buffer in a ray tracing shader
        RayTracingShaderReadSampledImageOrUniformTexelBuffer,
        /// Read as an input attachment with a color format in a ray tracing shader
        RayTracingShaderReadColorInputAttachment,
        /// Read as an input attachment with a depth/stencil format in a ray tracing shader
        RayTracingShaderReadDepthStencilInputAttachment,
        /// Read as an acceleration structure in a ray tracing shader
        RayTracingShaderReadAccelerationStructure,
        /// Read as any other resource in a ray tracing shader
        RayTracingShaderReadOther,

        /// Written as an acceleration structure during acceleration structure building
        AccelerationStructureBuildWrite,
        /// Read as an acceleration structure during acceleration structure building (e.g. a BLAS when building a TLAS)
        AccelerationStructureBuildRead,
        /// Written as a buffer during acceleration structure building (e.g. a staging buffer)
        AccelerationStructureBufferWrite,
    };

    namespace Barrier
    {
        EE_BASE_API bool IsCommonReadOnlyAccess( RenderResourceBarrierState const& access );
        EE_BASE_API bool IsCommonWriteAccess( RenderResourceBarrierState const& access );

        EE_BASE_API bool IsRasterReadOnlyAccess( RenderResourceBarrierState const& access );
        EE_BASE_API bool IsRasterWriteAccess( RenderResourceBarrierState const& access );
    }

    class RenderResourceAccessState
    {
    public:

        RenderResourceAccessState() = default;
        RenderResourceAccessState( RenderResourceBarrierState targetBarrier, bool skipSyncIfContinuous = false )
            : m_skipSyncIfContinuous( skipSyncIfContinuous ), m_targetBarrier( targetBarrier )
        {}

        inline void SetSkipSyncIfContinuous( bool enable )
        {
            m_skipSyncIfContinuous = enable;
        }

        inline bool GetSkipSyncIfContinuous() const
        {
            return m_skipSyncIfContinuous;
        }

        inline RenderResourceBarrierState GetCurrentAccess() const
        {
            return m_targetBarrier;
        }

        inline void TransiteTo( RenderResourceBarrierState state )
        {
            m_targetBarrier = state;
        }

    private:

        /// Skip resource synchronization between different passes.
        bool									m_skipSyncIfContinuous = false;
        RenderResourceBarrierState				m_targetBarrier = RenderResourceBarrierState::Undefined;
    };

    struct GlobalBarrier
    {
        uint32_t								m_previousAccessesCount;
        uint32_t								m_nextAccessesCount;
        RenderResourceBarrierState const*		m_pPreviousAccesses;
        RenderResourceBarrierState const*		m_pNextAccesses;
    };

    class RHIBuffer;
    class RHITexture;

    struct BufferBarrier
    {
        uint32_t								m_previousAccessesCount;
        uint32_t								m_nextAccessesCount;
        RenderResourceBarrierState const*		m_pPreviousAccesses;
        RenderResourceBarrierState const*		m_pNextAccesses;
        uint32_t								m_srcQueueFamilyIndex;
        uint32_t								m_dstQueueFamilyIndex;
        RHIBuffer*					            m_pRhiBuffer = nullptr;
        uint32_t								m_offset;
        uint32_t								m_size;
    };

    enum class TextureMemoryLayout
    {
        /// Choose the most optimal layout for each usage. Performs layout transitions as appropriate for the access.
        Optimal,
        /// Layout accessible by all Vulkan access types on a device - no layout transitions except for presentation
        General,
        /// Similar to `General`, but also allows presentation engines to access it - no layout transitions.
        /// Requires `VK_KHR_shared_presentable_image` to be enabled, and this can only be used for shared presentable
        /// images (i.e. single-buffered swap chains).
        GeneralAndPresentation,
    };

    enum class TextureAspectFlags
    {
        Color,
        Depth,
        Stencil,
        Metadata,
    };

    TBitFlags<TextureAspectFlags> PixelFormatToAspectFlags( EPixelFormat format );

    struct TextureSubresourceRange
    {
        TBitFlags<TextureAspectFlags>		    m_aspectFlags;
        uint32_t							    m_baseMipLevel;
        uint32_t							    m_levelCount;
        uint32_t							    m_baseArrayLayer;
        uint32_t							    m_layerCount;

        static TextureSubresourceRange AllSubresources( TBitFlags<TextureAspectFlags> aspectFlags )
        {
            TextureSubresourceRange range;
            range.m_aspectFlags = aspectFlags;
            range.m_baseMipLevel = 0;
            range.m_baseArrayLayer = 0;
            range.m_levelCount = ~(0u);
            range.m_layerCount = ~(0u);
            return range;
        }
    };

    // TextureSubresourceRangeUploadRef represent a mipmap of a set of mipmap chain in a texture as subresource.
    struct TextureSubresourceRangeUploadRef
    {
        RHIBuffer*                              m_pStagingBuffer = nullptr;
        TBitFlags<TextureAspectFlags>		    m_aspectFlags;
        uint32_t                                m_bufferBaseOffset = 0;
        uint32_t                                m_baseMipLevel = 0;
        uint32_t                                m_layer = 0;
        bool                                    m_uploadAllMips;
    };

    struct TextureBarrier
    {
        bool									m_discardContents;
        uint32_t								m_previousAccessesCount;
        uint32_t								m_nextAccessesCount;
        RenderResourceBarrierState const*		m_pPreviousAccesses;
        RenderResourceBarrierState const*		m_pNextAccesses;
        TextureMemoryLayout     				m_previousLayout;
        TextureMemoryLayout     				m_nextLayout;
        uint32_t								m_srcQueueFamilyIndex;
        uint32_t								m_dstQueueFamilyIndex;
        RHITexture*					            m_pRhiTexture = nullptr;
        TextureSubresourceRange					m_subresourceRange;
    };

    class RHIRenderPass;
    class RHIFramebuffer;
    class RHIPipelineState;

    struct RenderArea
    {
        uint32_t                                m_width;
        uint32_t                                m_height;
        int32_t                                 m_OffsetX;
        int32_t                                 m_OffsetY;

        inline bool IsValid() const
        {
            return m_width != 0
                && m_height != 0
                && static_cast<uint32_t>( Math::Abs( m_OffsetX ) ) <= m_width
                && static_cast<uint32_t>( Math::Abs( m_OffsetY ) ) <= m_height;
        }
    };

    class RHITextureView;

    // Used to synchronize CPU and GPU.
    class RHICPUGPUSync : public RHITaggedType
    {
    public:

        RHICPUGPUSync( ERHIType rhiType )
            : RHITaggedType( rhiType )
        {
        }
        virtual ~RHICPUGPUSync() = default;

        RHICPUGPUSync( RHICPUGPUSync const& ) = delete;
        RHICPUGPUSync& operator=( RHICPUGPUSync const& ) = delete;

        RHICPUGPUSync( RHICPUGPUSync&& ) = default;
        RHICPUGPUSync& operator=( RHICPUGPUSync&& ) = default;

    private:
    };

    class RHICommandBuffer : public RHITaggedType
    {
    public:

        RHICommandBuffer( ERHIType rhiType )
            : RHITaggedType( rhiType )
        {}
        virtual ~RHICommandBuffer() = default;

        RHICommandBuffer( RHICommandBuffer const& ) = delete;
        RHICommandBuffer& operator=( RHICommandBuffer const& ) = delete;

        RHICommandBuffer( RHICommandBuffer&& ) = default;
        RHICommandBuffer& operator=( RHICommandBuffer&& ) = default;

        // Render Commands
        //-------------------------------------------------------------------------

        virtual void Draw( uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0 ) = 0;
        virtual void DrawIndexed( uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0 ) = 0;

        virtual bool BeginRenderPass( RHIRenderPass* pRhiRenderPass, RHIFramebuffer* pFramebuffer, RenderArea const& renderArea, TSpan<RHITextureView*> textureViews ) = 0;
        virtual void EndRenderPass() = 0;

        virtual void PipelineBarrier(
            GlobalBarrier const* pGlobalBarriers,
            uint32_t bufferBarrierCount, BufferBarrier const* pBufferBarriers,
            uint32_t textureBarrierCount, TextureBarrier const* pTextureBarriers
        ) = 0;

        virtual void BindPipelineState( RHIPipelineState* pRhiPipelineState ) = 0;

        virtual void SetViewport( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) = 0;
        virtual void SetScissor( uint32_t width, uint32_t height, int32_t xOffset = 0, int32_t yOffset = 0 ) = 0;

        // TSpan<TextureSubresourceRangeUploadRef> represent the corresponding layer of a textures.
        // TSpan<TextureSubresourceRangeUploadRef>[0] is layer 0, and [1] is layer 1...
        // if this texture has only one layer, the size of uploadDataRef should be one.
        virtual void CopyBufferToTexture( RHITexture* pDstTexture, RenderResourceBarrierState dstBarrier, TSpan<TextureSubresourceRangeUploadRef> const uploadDataRef ) = 0;

    protected:

        inline void SetRecording( bool bIsRecording ) { m_bRecording = bIsRecording; }
        inline bool IsRecording() const { return m_bRecording; }

    private:
        
        bool                        m_bRecording = false;
    };

}