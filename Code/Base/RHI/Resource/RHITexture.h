#pragma once

#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"
#include "../RHICommandBuffer.h"
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

        inline RHITextureCreateDesc const& GetDesc() const { return m_desc; }

        RHITextureView* GetOrCreateView( RHIDevice* pDevice, RHITextureViewCreateDesc const& desc );
        void ClearAllViews( RHIDevice* pDevice );

        virtual void* MapSlice( RHIDevice* pDevice, uint32_t layer ) = 0;
        virtual void  UnmapSlice( RHIDevice* pDevice, uint32_t layer ) = 0;

        virtual bool UploadMappedData( RHIDevice* pDevice, RenderResourceBarrierState dstBarrierState ) = 0;

    protected:

        virtual RHITextureView* CreateView( RHIDevice* pDevice, RHITextureViewCreateDesc const& desc ) = 0;
        virtual void            DestroyView( RHIDevice* pDevice, RHITextureView* pTextureView ) = 0;

    protected:

        THashMap<RHITextureViewCreateDesc, RHITextureView*>         m_viewCache;
        RHITextureCreateDesc                                        m_desc;
    };
}