#pragma once

#include "Base/Render/RenderAPI.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Queue.h"
#include "Base/Resource/ResourcePtr.h"
#include "Resource/RHIDeferReleasable.h"
#include "Resource/RHIDescriptorSet.h"
#include "Resource/RHIResourceCreationCommons.h"
#include "RHITaggedType.h"

#include <EASTL/type_traits.h>

namespace EE::Render
{
    class Shader;

    class RenderWindow;
}

namespace EE::RHI
{
    class RHIShader;
    class RHISemaphore;
    class RHITexture;
    class RHIBuffer;
    class RHIRenderPass;
    class RHIPipelineState;

    class RHICPUGPUSync;
    class RHICommandBuffer;
    class RHICommandQueue;

    //-------------------------------------------------------------------------

    class DeferReleaseQueue
    {
        friend class RHIDescriptorPool;

    public:

        void ReleaseAllStaleResources( RHIDevice* pDevice );

    private:

        TQueue<RHIDescriptorPool>                   m_descriptorPools;
    };

    //-------------------------------------------------------------------------

    class RHIDevice : public RHITaggedType
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

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void   WaitUntilIdle() = 0;

        virtual size_t GetDeviceFrameIndex() const = 0;

        virtual RHICommandBuffer* AllocateCommandBuffer() = 0;
        virtual RHICommandQueue* GetMainGraphicCommandQueue() = 0;

        virtual RHICommandBuffer* GetImmediateCommandBuffer() = 0;

        virtual bool BeginCommandBuffer( RHICommandBuffer* pCommandBuffer ) = 0;
        virtual void EndCommandBuffer( RHICommandBuffer* pCommandBuffer ) = 0;

        virtual void SubmitCommandBuffer( RHICommandBuffer* pCommandBuffer ) = 0;

        //-------------------------------------------------------------------------

        virtual RHITexture* CreateTexture( RHITextureCreateDesc const& createDesc ) = 0;
        virtual void        DestroyTexture( RHITexture* pTexture ) = 0;

        virtual RHIBuffer* CreateBuffer( RHIBufferCreateDesc const& createDesc ) = 0;
        virtual void       DestroyBuffer( RHIBuffer* pBuffer ) = 0;

        virtual RHIShader* CreateShader( RHIShaderCreateDesc const& createDesc ) = 0;
        virtual void       DestroyShader( RHIShader* pShader ) = 0;

        virtual RHISemaphore* CreateSyncSemaphore( RHISemaphoreCreateDesc const& createDesc ) = 0;
        virtual void          DestroySyncSemaphore( RHISemaphore* pSemaphore ) = 0;

        //-------------------------------------------------------------------------

        virtual RHIRenderPass* CreateRenderPass( RHIRenderPassCreateDesc const& createDesc ) = 0;
        virtual void           DestroyRenderPass( RHIRenderPass* pRenderPass ) = 0;

        //-------------------------------------------------------------------------

        virtual RHIPipelineState* CreateRasterPipelineState( RHI::RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders ) = 0;
        virtual void              DestroyRasterPipelineState( RHIPipelineState* pPipelineState ) = 0;

        //-------------------------------------------------------------------------

        template <typename T>
        typename eastl::enable_if_t<eastl::is_same_v<T, RHIDescriptorPool>, void>
        DeferRelease( RHIDeferReleasable<T>& deferReleasable );

        //-------------------------------------------------------------------------

        void AdvanceFrame()
        {
            ++m_deviceFrameCount;
            m_deviceFrameIndex = ( m_deviceFrameIndex + 1 ) % NumDeviceFramebufferCount;
        }

        size_t GetTotalFrameCount() const { return m_deviceFrameCount; }
        uint32_t GetCurrentFrameIndex() const { return m_deviceFrameIndex; }

    protected:

        size_t                                      m_deviceFrameCount;
        uint32_t                                    m_deviceFrameIndex;

        DeferReleaseQueue                           m_deferReleaseQueues[NumDeviceFramebufferCount];
    };

    template <typename T>
    typename eastl::enable_if_t<eastl::is_same_v<T, RHIDescriptorPool>, void>
    RHIDevice::DeferRelease( RHIDeferReleasable<T>& deferReleasable )
    {
        uint32_t const frameIndex = GetCurrentFrameIndex();
        if constexpr ( eastl::is_same<T, RHIDescriptorPool>::value )
        {
            static_cast<T&>( deferReleasable ).Enqueue( m_deferReleaseQueues[frameIndex] );
        }
        else
        {
            EE_STATIC_ASSERT( false, "Test." );
        }
    }

}

