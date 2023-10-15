#pragma once

#include "RHITaggedType.h"
#include "Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
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
    };

}