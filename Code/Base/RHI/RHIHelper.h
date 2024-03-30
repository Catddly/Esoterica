#pragma once

#include "Base/Types/Function.h"
#include "Base/RHI/RHIObject.h"

namespace EE::RHI
{
    using ImmediateCommandCallback = TFunction<bool( RHICommandBufferRef& )>;

    // Dispatch a immediate command buffer to its command queue.
    // CPU will wait until GPU finished this command.
    void DispatchImmediateGraphicCommandAndWait( RHIDeviceRef& pDevice, ImmediateCommandCallback const& commandCallback );

    void DispatchImmediateTransferCommandAndWait( RHIDeviceRef& pDevice, ImmediateCommandCallback const& commandCallback );
}