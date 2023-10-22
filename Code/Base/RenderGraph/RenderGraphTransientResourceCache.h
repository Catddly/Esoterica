#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Types/HashMap.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIDevice;
    class RHIBuffer;
    class RHITexture;
}

namespace EE::RG
{
    class RGTransientResourceCache
    {
    public:

        RGTransientResourceCache() = default;

        RGTransientResourceCache( RGTransientResourceCache const& ) = delete;
        RGTransientResourceCache& operator=( RGTransientResourceCache const& ) = delete;

        RGTransientResourceCache( RGTransientResourceCache&& ) noexcept = default;
        RGTransientResourceCache& operator=( RGTransientResourceCache&& ) noexcept = default;

    public:

        RHI::RHIBuffer* FetchAvailableBuffer( RHI::RHIBufferCreateDesc const& bufferDesc );
        RHI::RHITexture* FetchAvailableTexture( RHI::RHITextureCreateDesc const& textureDesc );

        void StoreBufferResource( RHI::RHIBuffer* pBuffer );
        void StoreTextureResource( RHI::RHITexture* pTexture );

        void DestroyAllResource( RHI::RHIDevice* pDevice );

    private:

        THashMap<RHI::RHIBufferCreateDesc, TVector<RHI::RHIBuffer*>>        m_bufferCache;
        THashMap<RHI::RHITextureCreateDesc, TVector<RHI::RHITexture*>>      m_textureCache;
    };
}