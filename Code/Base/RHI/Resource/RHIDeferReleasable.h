#pragma once

namespace EE::RHI
{
    class RHIDevice;
    class DeferReleaseQueue;

    // CRTP type to define defer releasable stack handle resources.
    template <typename T>
    class RHIDeferReleasable
    {
        friend class RHIDevice;

    public:

        void Enqueue( DeferReleaseQueue& queue ) { EE_UNREACHABLE_CODE(); }
    };
}