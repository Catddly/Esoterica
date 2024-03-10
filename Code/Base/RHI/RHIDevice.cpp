#include "RHIDevice.h"
#include "Resource/RHIDescriptorSet.h"
#include "Resource/RHIBuffer.h"
#include "Resource/RHITexture.h"

namespace EE::RHI
{
    //-------------------------------------------------------------------------

    void DeferReleaseQueue::ReleaseAllStaleResources( RHIDevice* pDevice )
    {
        RHIDescriptorPool popPool;
        while ( m_descriptorPools.try_dequeue( popPool ) )
        {
            //auto& pool = m_descriptorPools.front();
            popPool.Release( pDevice );

            //m_descriptorPools.pop();
        }

        RHIBuffer* pPopBuffer;
        while ( m_deferReleaseBuffers.try_dequeue( pPopBuffer ) )
        {
            pDevice->DestroyBuffer( pPopBuffer );
        }

        RHITexture* pPopTexture;
        while ( m_deferReleaseTextures.try_dequeue( pPopTexture ) )
        {
            pDevice->DestroyTexture( pPopTexture );
        }
    }

    //-------------------------------------------------------------------------

    template <>
    void RHIDevice::DeferRelease( RHIStaticDeferReleasable<RHIDescriptorPool>& deferReleasable )
    {
        uint32_t const frameIndex = GetDeviceFrameIndex();
        auto& concreteType = static_cast<RHIDescriptorPool&>( deferReleasable );
        concreteType.Enqueue( m_deferReleaseQueues[frameIndex] );
    }

    void RHIDevice::DeferRelease( RHIDynamicDeferReleasable* pDeferReleasable )
    {
        if ( pDeferReleasable )
        {
            uint32_t const frameIndex = GetDeviceFrameIndex();
            pDeferReleasable->Enqueue( m_deferReleaseQueues[frameIndex] );
        }
    }
}