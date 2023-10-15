#include "RenderGraphResourceRegistry.h"
#include "Base/Threading/Threading.h"

namespace EE::RG
{
    void RGResourceRegistry::Compile( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( pDevice != nullptr );
        EE_ASSERT( m_resourceState == ResourceState::Registering );

        m_compiledResources.reserve( m_registeredResources.size() );

        for ( auto& rgResource : m_registeredResources )
        {
            m_compiledResources.emplace_back( eastl::exchange( rgResource, {} ).Compile( pDevice ) );
        }

        m_registeredResources.clear();
        m_resourceState = ResourceState::Compiled;
    }

    void RGResourceRegistry::ClearAll( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( pDevice != nullptr );

        for ( auto& resource : m_compiledResources )
        {
            resource.Retire( pDevice );
        }

        m_registeredResources.clear();
        m_compiledResources.clear();
    }
}