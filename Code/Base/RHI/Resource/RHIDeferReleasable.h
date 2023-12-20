#pragma once

#include "Base/Esoterica.h"
#include "Base/_Module/API.h"

namespace EE::RHI
{
    class RHIDevice;
    class DeferReleaseQueue;

    // CRTP type to define static defer releasable stack handle resources.
    template <typename T>
    class RHIStaticDeferReleasable
    {
        friend class RHIDevice;

    public:

        void Enqueue( DeferReleaseQueue& queue ) { EE_UNREACHABLE_CODE(); }
    };

    class EE_BASE_API RHIDynamicDeferReleasable
    {
    public:

        virtual ~RHIDynamicDeferReleasable() = default;

    public:

        virtual void Enqueue( DeferReleaseQueue& queue ) = 0;

        virtual void Release( RHIDevice* pDevice ) = 0;
    };
}