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
        typename B = typename HelperTemplates::GetRHIBaseClass<T>::Type,
        typename... Args
    >
    inline TTSSharedPtr<B> MakeRHIObject( Args&&... args )
    {
        T* pNew = EE::New<T>( eastl::forward<Args>( args )... );
        return TTSSharedPtr<B>( pNew );
    }

    // Forward definitions
    //-------------------------------------------------------------------------

    class RHIDevice;
    class RHISwapchain;

    class RHIBuffer;
    class RHITexture;

    class RHICommandQueue;
    class RHICommandBuffer;

    using RHIDeviceRef = TTSSharedPtr<RHIDevice>;
    using RHISwapchainRef = TTSSharedPtr<RHISwapchain>;

    using RHIBufferRef = TTSSharedPtr<RHIBuffer>;
    using RHITextureRef = TTSSharedPtr<RHITexture>;

    using RHICommandQueueRef = TTSSharedPtr<RHICommandQueue>;
    using RHICommandBufferRef = TTSSharedPtr<RHICommandBuffer>;

}