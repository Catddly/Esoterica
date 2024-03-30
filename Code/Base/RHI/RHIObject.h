#pragma once

#include "Base/Memory/Pointers.h"
#include "Base/RHI/RHITaggedType.h"

// TODO: use C++20 concept to rewrite this

//-------------------------------------------------------------------------

// helper macro
// TODO: check to see if reflection system can help with

//#define EE_RHI_STATIC_TAGGED_TYPE( rhiType, rhiBaseType ) inline static EE::RHI::ERHIType GetStaticRHIType() { return rhiType; } \
//    using StaticRHIBaseClass = EE::RHI::rhiBaseType;

// TODO: reduce parameters
//       concat rhiType to rhiBaseType
//       e.g. RHI::ERHIType::Vulkan + RHIDevice = VulkanDevice

#define EE_RHI_OBJECT( rhiType, rhiBaseType ) public: \
    inline static ::EE::RHI::ERHIType GetStaticRHIType() { return ::EE::RHI::ERHIType::rhiType; } \
    using StaticRHIBaseClass = typename ::EE::RHI::rhiBaseType; \
private: \

//-------------------------------------------------------------------------

namespace EE::RHI
{
    namespace HelperTemplates
    {
        template <typename T>
        struct GetRHIBaseClass
        {
            using Type = typename T::StaticRHIBaseClass;
        };
    }

    template <
        typename T,
        typename... Args
    >
    inline auto MakeRHIObject( Args&&... args )
    {
        return TSharedPtr( EE::New<T>( eastl::forward<Args>( args )... ) );
    }

    // Forward definitions
    //-------------------------------------------------------------------------

    class RHIDevice;
    class RHISwapchain;

    class RHIBuffer;
    class RHITexture;

    class RHISemaphore;
    class RHICommandQueue;
    class RHICommandBufferPool;
    class RHICommandBuffer;

    class RHIRenderPass;
    class RHIFramebuffer;

    class RHIShader;
    class RHIPipelineState;
    class RHIRasterPipelineState;
    class RHIComputePipelineState;

    using RHIDeviceRef = TSharedPtr<RHIDevice>;
    using RHISwapchainRef = TSharedPtr<RHISwapchain>;

    using RHIBufferRef = TSharedPtr<RHIBuffer>;
    using RHITextureRef = TSharedPtr<RHITexture>;

    using RHISemaphoreRef = TSharedPtr<RHISemaphore>;
    using RHICommandQueueRef = TSharedPtr<RHICommandQueue>;
    using RHICommandBufferPoolRef = TSharedPtr<RHICommandBufferPool>;
    using RHICommandBufferRef = TSharedPtr<RHICommandBuffer>;

    using RHIRenderPassRef = TSharedPtr<RHIRenderPass>;
    using RHIFramebufferRef = TSharedPtr<RHIFramebuffer>;

    using RHIShaderRef = TSharedPtr<RHIShader>;
    using RHIPipelineRef = TSharedPtr<RHIPipelineState>;
    using RHIRasterPipelineRef = TSharedPtr<RHIRasterPipelineState>;
    using RHIComputePipelineRef = TSharedPtr<RHIComputePipelineState>;

}