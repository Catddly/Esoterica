#include "RenderGraphResourceRegistry.h"
#include "RenderGraphResolver.h"
#include "Base/Threading/Threading.h"

namespace EE::RG
{
    bool RGResourceRegistry::Compile( RHI::RHIDevice* pDevice, RGResolveResult const& result )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_resourceState == ResourceState::Registering );

        if ( !pDevice )
        {
            return false;
        }

        m_compiledResources.reserve( m_registeredResources.size() );

        for ( uint32_t i = 0; i < static_cast<uint32_t>( m_registeredResources.size() ); ++i )
        {
            auto& rgResource = m_registeredResources[i];

            auto iterator = result.m_resourceLifetimes.find( i );
            if ( iterator != result.m_resourceLifetimes.end() )
            {
                auto& compiledResource = m_compiledResources.emplace_back( eastl::move( rgResource ).Compile( pDevice, *this, m_transientResourceCache ) );
                compiledResource.m_lifetime = iterator->second;
            }
            else
            {
                // add a empty compiled resource, since this resource is not reference by any node,
                // it will NOT be access by outer class.
                m_compiledResources.emplace_back();
            }
        }

        EE_ASSERT( m_registeredResources.size() == m_compiledResources.size() );

        m_registeredResources.clear();
        m_resourceState = ResourceState::Compiled;

        return true;
    }

    void RGResourceRegistry::Retire()
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( m_resourceState == ResourceState::Compiled );

        for ( auto& resource : m_compiledResources )
        {
            if ( resource.m_lifetime.HasValidLifetime() )
            {
                resource.Retire( *this, m_transientResourceCache );
            }
        }

        m_registeredResources.clear();
        m_compiledResources.clear();
        m_exportableResources.clear();
        // one frame draw end, render graph go back to origin state
        m_resourceState = ResourceState::Registering;
    }

    void RGResourceRegistry::Shutdown( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( pDevice != nullptr );
        
        if ( !m_compiledResources.empty() )
        {
            EE_LOG_FATAL_ERROR( "RenderGraph", "RGResourceRegistry", "Shutdown() had been called before ReleaseAllResources(), please release all resources before shutdown!" );
        }

        m_transientResourceCache.DestroyAllResource( pDevice );
    }
}