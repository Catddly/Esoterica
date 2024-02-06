#pragma once

#include "RHITaggedType.h"

namespace EE::RHI
{
    enum class CommandQueueType : uint8_t
    {
        Graphic = 0,
        Compute,
        Transfer,
        Unknown
    };

    class RHICommandQueue : public RHITaggedType
    {
    public:

        RHICommandQueue( ERHIType rhiType )
            : RHITaggedType( rhiType )
        {
        }
        virtual ~RHICommandQueue() = default;

        RHICommandQueue( RHICommandQueue const& ) = delete;
        RHICommandQueue& operator=( RHICommandQueue const& ) = delete;

        RHICommandQueue( RHICommandQueue&& ) = default;
        RHICommandQueue& operator=( RHICommandQueue&& ) = default;

        //-------------------------------------------------------------------------

        // Note: In vulkan, this will return this queue family index of this queue.
        virtual uint32_t GetDeviceIndex() const = 0;
        virtual CommandQueueType GetType() const = 0;

        virtual void WaitUntilIdle() const = 0;

    private:

    };

}