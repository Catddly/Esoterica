#pragma once

#include "Base/Memory/Pointers.h"
#include "RHIResource.h"

namespace EE::RHI
{
    class EE_BASE_API RHISemaphore : public RHIResource
    {
    public:

        RHISemaphore( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHISemaphore() = default;
    };

    //-------------------------------------------------------------------------

    using RHISemaphoreRef = TTSSharedPtr<RHISemaphore>;
}