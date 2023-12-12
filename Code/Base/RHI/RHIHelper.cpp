#include "RHIHelper.h"
#include "RHIDevice.h"
#include "RHICommandBuffer.h"
#include "Base/Memory/Memory.h"

namespace EE::RHI
{
    void DispatchImmediateCommandAndWait( RHIDevice* pDevice, ImmediateCommandCallback const& commandCallback )
    {
        EE_ASSERT( pDevice );

        auto* pCommandBuffer = pDevice->GetImmediateCommandBuffer();

        if ( pCommandBuffer )
        {
            EE_ASSERT( pDevice->BeginCommandBuffer( pCommandBuffer ) );

            EE_ASSERT( commandCallback( pCommandBuffer ) );

            pDevice->EndCommandBuffer( pCommandBuffer );

            pDevice->SubmitCommandBuffer( pCommandBuffer );
            pDevice->WaitUntilIdle();
        }
    }
}