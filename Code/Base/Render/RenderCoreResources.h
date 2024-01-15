#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::RHI { class RHITexture; }

namespace EE::Render
{
    class RenderDevice;

    //-------------------------------------------------------------------------

    namespace CoreResources
    {
        void Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );

        EE_BASE_API RHI::RHITexture* GetMissingTexture();
    };
}