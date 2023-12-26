#include "RenderGraphResolver.h"
#include "RenderGraphResourceRegistry.h"
#include "Base/Logging/Log.h"

namespace EE::RG
{
    RenderGraphResolver::RenderGraphResolver( TVector<RGNode> const& graph, RGResourceRegistry& resourceRegistry )
        : m_graph( graph ), m_resourceRegistry( resourceRegistry )
    {}

    RGResolveResult RenderGraphResolver::Resolve()
    {
        RGResolveResult result;

        if ( !m_graphResolved )
        {
            auto const& registeredResource = m_resourceRegistry.GetRegisteredResources();

            int32_t currentNodeIndex = 0;
            for ( auto const& node : m_graph )
            {
                for ( auto const& input : node.m_inputs )
                {
                    auto iterator = result.m_resourceLifetimes.find( input.m_slotID.m_id );
                    if ( iterator == result.m_resourceLifetimes.end() )
                    {
                        auto& lifetime = result.m_resourceLifetimes.insert( input.m_slotID.m_id ).first->second;
                        if ( m_resourceRegistry.IsExportableResource( input.m_slotID ) )
                        {
                            lifetime = RGResourceLifetime{ currentNodeIndex, ExportableResourceLifetime };
                        }
                        else
                        {
                            lifetime = RGResourceLifetime{ currentNodeIndex, currentNodeIndex };
                        }
                    }
                    else
                    {
                        auto& lifetime = iterator->second;
                        if ( m_resourceRegistry.IsExportableResource( input.m_slotID ) )
                        {
                            lifetime.m_lifeEndTimePoint = ExportableResourceLifetime;
                        }
                        else
                        {
                            EE_ASSERT( lifetime.HasValidLifetime() );

                            lifetime.m_lifeEndTimePoint = currentNodeIndex;
                        }
                    }
                }

                for ( auto const& output : node.m_outputs )
                {
                    auto iterator = result.m_resourceLifetimes.find( output.m_slotID.m_id );
                    if ( iterator == result.m_resourceLifetimes.end() )
                    {
                        auto& lifetime = result.m_resourceLifetimes.insert( output.m_slotID.m_id ).first->second;
                        if ( m_resourceRegistry.IsExportableResource( output.m_slotID ) )
                        {
                            lifetime = RGResourceLifetime{ currentNodeIndex, ExportableResourceLifetime };
                        }
                        else
                        {
                            lifetime = RGResourceLifetime{ currentNodeIndex, currentNodeIndex };
                        }
                    }
                    else
                    {
                        auto& lifetime = iterator->second;

                        if ( m_resourceRegistry.IsExportableResource( output.m_slotID ) )
                        {
                            lifetime.m_lifeEndTimePoint = ExportableResourceLifetime;
                        }
                        else
                        {
                            EE_ASSERT( lifetime.HasValidLifetime() );

                            lifetime.m_lifeEndTimePoint = currentNodeIndex;
                        }
                    }
                }

                ++currentNodeIndex;
            }
        }
        else
        {
            EE_LOG_WARNING( "RenderGraph", "RenderGraphResolver", "Render Graph had been resolved." );
        }

        return result;
    }
}