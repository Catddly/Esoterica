#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Types/HashMap.h"
#include "RenderGraphResource.h"
#include "RenderGraphNode.h"

namespace EE::RG
{
    class RGResourceRegistry;

    struct RGResolveResult
    {
        // TODO: performance (sequence access in hashmap)
        THashMap<uint32_t, RGResourceLifetime>          m_resourceLifetimes;
    };

    // Used to analyze render graph resources dependency and export a linear command queue
    // dependent execution sequence.
    class RenderGraphResolver
    {
    public:

        // TODO: this will be changed lately when we use a real graph
        RenderGraphResolver( TVector<RGNode> const& graph, RGResourceRegistry& resourceRegistry );

        RenderGraphResolver( RenderGraphResolver const& ) = delete;
        RenderGraphResolver& operator=( RenderGraphResolver const& ) = delete;

        RenderGraphResolver( RenderGraphResolver&& ) = delete;
        RenderGraphResolver& operator=( RenderGraphResolver&& ) = delete;

        RGResolveResult Resolve();

    private:

        TVector<RGNode> const&                  m_graph;
        RGResourceRegistry&                     m_resourceRegistry;

        bool                                    m_graphResolved = false;
    };
}