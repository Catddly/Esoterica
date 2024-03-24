#include "RHIBuffer.h"
#include "../RHIDevice.h"

namespace EE::RHI
{
    void RHIBuffer::Enqueue( DeferReleaseQueue& queue )
    {
        queue.m_deferReleaseBuffers.enqueue( this );
    }

    void RHIBuffer::Release( RHIDeviceRef& pDevice )
    {
        pDevice->DestroyBuffer( this );
    }
}