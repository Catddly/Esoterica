#pragma once

#include "Base/_Module/API.h"
#include "Base/Memory/Pointers.h"
#include "../RHIObject.h"
#include "../RHICommandBuffer.h"
#include "RHIResource.h"
#include "RHIDeferReleasable.h"
#include "RHITextureView.h"
#include "RHIResourceCreationCommons.h"
#include "Base/Types/HashMap.h"

namespace EE::RHI
{
    class DeferReleaseQueue;

    class RHITexture : public RHIResource, public RHIDynamicDeferReleasable
    {
        friend class DeferReleaseQueue;

    public:

        RHITexture( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHITexture();

    public:

        inline RHITextureCreateDesc const& GetDesc() const { return m_desc; }

        EE_BASE_API RHITextureView GetOrCreateView( RHIDeviceRef& pDevice, RHITextureViewCreateDesc const& desc );
        void ClearAllViews( RHIDeviceRef& pDevice );

        virtual void* MapSlice( RHIDeviceRef& pDevice, uint32_t layer ) = 0;
        virtual void  UnmapSlice( RHIDeviceRef& pDevice, uint32_t layer ) = 0;

        virtual bool UploadMappedData( RHIDeviceRef& pDevice, RenderResourceBarrierState dstBarrierState ) = 0;

    protected:

        virtual RHITextureView CreateView( RHIDeviceRef& pDevice, RHITextureViewCreateDesc const& desc ) const = 0;
        virtual void           DestroyView( RHIDeviceRef& pDevice, RHITextureView& textureView ) = 0;

    private:

        virtual void Enqueue( DeferReleaseQueue& queue ) override;

        inline virtual void Release( RHIDeviceRef& pDevice ) override;

    protected:

        THashMap<RHITextureViewCreateDesc, RHITextureView>              m_viewCache;
        RHITextureCreateDesc                                            m_desc;
    };
}