#include "RHIHelper.h"
#include "RHIDevice.h"
#include "RHICommandBuffer.h"
#include "Base/Memory/Memory.h"

namespace EE::RHI
{
    void DispatchImmediateGraphicCommandAndWait( RHIDeviceRef& pDevice, ImmediateCommandCallback const& commandCallback )
    {
        EE_ASSERT( pDevice );
        
        auto pCommandBuffer = pDevice->GetImmediateGraphicCommandBuffer();

        if ( pCommandBuffer )
        {
            EE_ASSERT( pDevice->BeginCommandBuffer( pCommandBuffer ) );
            EE_ASSERT( commandCallback( pCommandBuffer ) );
            pDevice->EndCommandBuffer( pCommandBuffer );

            pDevice->SubmitCommandBuffer( pCommandBuffer, {}, {}, {} );
            
            // TODO: GPU sync primitives
            //pDevice->WaitUntilIdle();
        }
    }

    void DispatchImmediateTransferCommandAndWait( RHIDeviceRef& pDevice, ImmediateCommandCallback const& commandCallback )
    {
        EE_ASSERT( pDevice );

        auto pCommandBuffer = pDevice->GetImmediateTransferCommandBuffer();

        if ( pCommandBuffer )
        {
            EE_ASSERT( pDevice->BeginCommandBuffer( pCommandBuffer ) );
            EE_ASSERT( commandCallback( pCommandBuffer ) );
            pDevice->EndCommandBuffer( pCommandBuffer );

            pDevice->SubmitCommandBuffer( pCommandBuffer, {}, {}, {} );

            // TODO: GPU sync primitives
            //pDevice->WaitUntilIdle();
        }
    }

}