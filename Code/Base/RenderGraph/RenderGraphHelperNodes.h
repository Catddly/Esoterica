#pragma once

#include "Base/Esoterica.h"
#include "RenderGraphResource.h"

namespace EE::RG
{
    class RenderGraph;

    template <typename Tag>
    class RGResourceHandle;

    EE_BASE_API void AddClearDepthStencilNode( RenderGraph& renderGraph, RGResourceHandle<RGResourceTagTexture>& depthTexture );
}