#include "RenderGraphHelperNodes.h"
#include "RenderGraph.h"
#include "RenderGraphResourceRegistry.h"

namespace EE::RG
{
    void AddClearDepthStencilNode( RenderGraph& renderGraph, RGResourceHandle<RGResourceTagTexture>& depthTexture )
    {
        auto node = renderGraph.AddNode( "Clear Depth Stencil" );

        auto depthTextureBinding = node.CommonWrite( depthTexture, RHI::RenderResourceBarrierState::TransferWrite );

        node.Execute( [=] ( RGRenderCommandContext& context )
        {
            context.ClearDepthStencil( depthTextureBinding );
        });
    }
}