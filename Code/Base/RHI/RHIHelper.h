#pragma once

#include "Base/Types/Function.h"

namespace EE::RHI
{
    class RHIDevice;
    class RHICommandBuffer;

    using ImmediateCommandCallback = TFunction<bool( RHICommandBuffer* )>;

    // Dispatch a immediate command buffer to its command queue.
    // CPU will wait until GPU finished this command.
    void DispatchImmediateCommandAndWait( RHIDevice* pDevice, ImmediateCommandCallback const& commandCallback );
}