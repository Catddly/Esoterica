#pragma once

#include "RenderGraphNodeRef.h"

namespace EE::RG
{
    void ClearColor();

    template <RGResourceViewType RVT>
    void ClearDepthStencil( RGNodeResourceRef<RGResourceTagTexture, RVT>& clearDepthTexture, float depthValue );

    //-------------------------------------------------------------------------

    template <RGResourceViewType RVT>
    void ClearDepthStencil( RGNodeResourceRef<RGResourceTagTexture, RVT>& clearDepthTexture, float depthValue )
    {

    }
}