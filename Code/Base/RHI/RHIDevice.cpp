#include "RHIDevice.h"

namespace EE::RHI
{
    void DeferReleaseQueue::ReleaseAllStaleResources( RHIDevice* pDevice )
    {
        while ( !m_descriptorPools.empty() )
        {
            auto& pool = m_descriptorPools.front();
            pool.Release( pDevice );

            m_descriptorPools.pop();
        }
    }
}