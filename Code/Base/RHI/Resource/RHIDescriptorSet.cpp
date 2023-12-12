#include "RHIDescriptorSet.h"
#include "../RHIDevice.h"

namespace EE::RHI
{
    void RHIDescriptorPool::Enqueue( DeferReleaseQueue& queue )
    {
        if ( IsValid() )
        {
            queue.m_descriptorPools.push( *this );
        }
    }
}