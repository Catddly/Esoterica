#pragma once

#include "RHITaggedType.h"
#include "RHIObject.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Queue.h"
#include "Base/Threading/Threading.h"
#include "Base/Resource/ResourcePtr.h"
#include "Resource/RHIDeferReleasable.h"
#include "Resource/RHIDescriptorSet.h"
#include "Resource/RHIResourceCreationCommons.h"

#include <EASTL/type_traits.h>

namespace EE::Render
{
    class Shader;

    class RenderWindow;
}

namespace EE::RHI
{
    class RHICPUGPUSync;

    //-------------------------------------------------------------------------

    class DeferReleaseQueue
    {
        friend class RHIDescriptorPool;
        friend class RHIBuffer;
        friend class RHITexture;

    public:

        void ReleaseAllStaleResources( RHIDeviceRef& pDevice );

    private:

        Threading::LockFreeQueue<RHIDescriptorPool>   m_descriptorPools;
        Threading::LockFreeQueue<RHIBufferRef>        m_deferReleaseBuffers;
        Threading::LockFreeQueue<RHITextureRef>       m_deferReleaseTextures;
    };

    //-------------------------------------------------------------------------

    class RHIDevice : public RHITaggedType, public TSharedFromThis<RHIDevice>
    {
    public:

        static constexpr size_t NumDeviceFramebufferCount = 2;

    public:

        using CompiledShaderArray = TFixedVector<Render::Shader const*, static_cast<size_t>( Render::NumPipelineStages )>;

        //-------------------------------------------------------------------------

        RHIDevice( ERHIType rhiType = ERHIType::Invalid )
            : RHITaggedType( rhiType ), m_deviceFrameCount( 0 ), m_deviceFrameIndex( 0 )
        {}
        virtual ~RHIDevice() = default;

        RHIDevice( RHIDevice const& ) = delete;
        RHIDevice& operator=( RHIDevice const& ) = delete;

        RHIDevice( RHIDevice&& ) = default;
        RHIDevice& operator=( RHIDevice&& ) = default;

        //-------------------------------------------------------------------------

        //virtual CreateSecondaryRenderWindow( RenderWindow&  );

        //-------------------------------------------------------------------------

        // Begin device frame, call at the beginning of drawing loop.
        virtual void BeginFrame() = 0;
        // End device frame, call when your device frame is end.
        virtual void EndFrame() = 0;

        virtual void WaitUntilIdle() = 0;

        virtual RHICommandBufferRef AllocateCommandBuffer() = 0;
        virtual RHICommandQueueRef GetMainGraphicCommandQueue() = 0;

        virtual RHICommandBufferRef GetImmediateGraphicCommandBuffer() = 0;
        virtual RHICommandBufferRef GetImmediateTransferCommandBuffer() = 0;

        virtual bool BeginCommandBuffer( RHICommandBufferRef& pCommandBuffer ) = 0;
        virtual void EndCommandBuffer( RHICommandBufferRef& pCommandBuffer ) = 0;

        virtual void SubmitCommandBuffer(
            RHICommandBufferRef& pCommandBuffer,
            TSpan<RHISemaphoreRef&> pWaitSemaphores,
            TSpan<RHISemaphoreRef&> pSignalSemaphores,
            TSpan<Render::PipelineStage> waitStages
        ) = 0;

        //-------------------------------------------------------------------------

        virtual RHITextureRef CreateTexture( RHITextureCreateDesc const& createDesc ) = 0;
        virtual void          DestroyTexture( RHITextureRef& pTexture ) = 0;

        virtual RHIBufferRef CreateBuffer( RHIBufferCreateDesc const& createDesc ) = 0;
        virtual void         DestroyBuffer( RHIBufferRef& pBuffer ) = 0;

        virtual RHIShaderRef CreateShader( RHIShaderCreateDesc const& createDesc ) = 0;
        virtual void         DestroyShader( RHIShaderRef& pShader ) = 0;

        virtual RHISemaphoreRef CreateSyncSemaphore( RHISemaphoreCreateDesc const& createDesc ) = 0;
        virtual void            DestroySyncSemaphore( RHISemaphoreRef& pSemaphore ) = 0;

        //-------------------------------------------------------------------------

        virtual RHIRenderPassRef CreateRenderPass( RHIRenderPassCreateDesc const& createDesc ) = 0;
        virtual void             DestroyRenderPass( RHIRenderPassRef& pRenderPass ) = 0;

        //-------------------------------------------------------------------------

        virtual RHIPipelineRef CreateRasterPipeline( RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders ) = 0;
        virtual void           DestroyRasterPipeline( RHIPipelineRef& pPipelineState ) = 0;

        virtual RHIPipelineRef CreateComputePipeline( RHIComputePipelineStateCreateDesc const& createDesc, Render::ComputeShader const* pCompiledShader ) = 0;
        virtual void           DestroyComputePipeline( RHIPipelineRef& pPipelineState ) = 0;

        //-------------------------------------------------------------------------

        template <typename T>
        typename eastl::enable_if_t<eastl::is_same_v<T, RHIDescriptorPool>, void>
        DeferRelease( RHIStaticDeferReleasable<T>& deferReleasable );

        EE_BASE_API void DeferRelease( RHIDynamicDeferReleasable* pDeferReleasable );

        //-------------------------------------------------------------------------

        inline void AdvanceFrame()
        {
            ++m_deviceFrameCount;
            m_deviceFrameIndex = ( m_deviceFrameIndex + 1 ) % NumDeviceFramebufferCount;
        }

        size_t GetDeviceFrameCount() const { return m_deviceFrameCount; }
        uint32_t GetDeviceFrameIndex() const { return m_deviceFrameIndex; }

    protected:

        size_t                                      m_deviceFrameCount;
        uint32_t                                    m_deviceFrameIndex;

        // TODO: replace to lock free queue
        DeferReleaseQueue                           m_deferReleaseQueues[NumDeviceFramebufferCount];
    };
}

