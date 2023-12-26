#include "RHIDevice.h"
#include "Resource/RHIDescriptorSet.h"
#include "Resource/RHIBuffer.h"
#include "Resource/RHITexture.h"

namespace EE::RHI
{
    //-------------------------------------------------------------------------

    void DeferReleaseQueue::ReleaseAllStaleResources( RHIDevice* pDevice )
    {
        while ( !m_descriptorPools.empty() )
        {
            auto& pool = m_descriptorPools.front();
            pool.Release( pDevice );

            m_descriptorPools.pop();
        }

        while ( !m_deferReleaseBuffers.empty() )
        {
            auto& buffer = m_deferReleaseBuffers.front();
            pDevice->DestroyBuffer( buffer );
            buffer = nullptr;

            m_deferReleaseBuffers.pop();
        }

        while ( !m_deferReleaseTextures.empty() )
        {
            auto& texture = m_deferReleaseTextures.front();
            pDevice->DestroyTexture( texture );
            texture = nullptr;

            m_deferReleaseTextures.pop();
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