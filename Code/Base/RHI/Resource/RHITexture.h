#pragma once

#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"
#include "Base/Types/HashMap.h"

namespace EE::RHI
{
    class RHIDevice;

    class RHITextureView : public RHIResourceView
    {
    public:

        RHITextureView( ERHIType rhiType = ERHIType::Invalid )
            : RHIResourceView( rhiType )
        {}
        virtual ~RHITextureView() = default;
    };

    class RHITexture : public RHIResource
    {
    public:

        RHITexture( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHITexture();

    public:

        inline RHITextureCreateDesc GetDesc() const { return m_desc; }

        RHITextureView* GetOrCreateView( RHIDevice* pDevice, RHITextureViewCreateDesc const& desc );
        void ClearAllViews( RHIDevice* pDevice );

    protected:

        virtual RHITextureView* CreateView( RHIDevice* pDevice, RHITextureViewCreateDesc const& desc ) = 0;
        virtual void            DestroyView( RHIDevice* pDevice, RHITextureView* pTextureView ) = 0;

    protected:

        THashMap<RHITextureViewCreateDesc, RHITextureView*>         m_viewCache;
        RHITextureCreateDesc                                        m_desc;
    };
}