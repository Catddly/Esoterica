#pragma once

#include "RHITaggedType.h"
#include "Base/Types/Arrays.h"
#include "Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHITexture;

    class RHISwapchain : public RHITaggedType
    {
    public:

        RHISwapchain( ERHIType rhiType )
            : RHITaggedType( rhiType )
        {}
        virtual ~RHISwapchain() = default;

        RHISwapchain( RHISwapchain const& ) = delete;
        RHISwapchain& operator=( RHISwapchain const& ) = delete;

        RHISwapchain( RHISwapchain&& ) = default;
        RHISwapchain& operator=( RHISwapchain&& ) = default;

    public:

        virtual RHI::RHITextureCreateDesc GetPresentTextureDesc() const = 0;
        virtual TVector<RHI::RHITexture const*> const GetPresentTextures() const = 0;
    };

}