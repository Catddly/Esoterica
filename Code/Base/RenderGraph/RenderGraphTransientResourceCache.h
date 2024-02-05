#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Types/HashMap.h"
#include "Base/Types/String.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIBuffer;
    class RHITexture;
}

namespace EE::Render
{
    class RenderDevice;
}

//-------------------------------------------------------------------------

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

        RHI::RHIBuffer* FetchAvailableTemporaryBuffer( RHI::RHIBufferCreateDesc const& bufferDesc );
        RHI::RHITexture* FetchAvailableTemporaryTexture( RHI::RHITextureCreateDesc const& textureDesc );

        // If the named buffer exists and it is updated successfully, return true. Otherwise, return false.
        // Buffer will be updated immediately.
        bool UpdateDirtyNamedBuffer( String const& name, Render::RenderDevice* pDevice, RHI::RHIBufferCreateDesc const& bufferDesc );
        // If the named texture exists and it is updated successfully, return true. Otherwise, return false.
        // Texture will be updated immediately.
        bool UpdateDirtyNamedTexture( String const& name, Render::RenderDevice* pDevice, RHI::RHITextureCreateDesc const& textureDesc );

        RHI::RHIBuffer* GetOrCreateNamedBuffer( String const& name, Render::RenderDevice* pDevice, RHI::RHIBufferCreateDesc const& bufferDesc );
        RHI::RHITexture* GetOrCreateNamedTexture( String const& name, Render::RenderDevice* pDevice, RHI::RHITextureCreateDesc const& textureDesc );

        //-------------------------------------------------------------------------

        void RestoreBuffer( RHI::RHIBuffer* pBuffer );
        void RestoreTexture( RHI::RHITexture* pTexture );

        //-------------------------------------------------------------------------

        void DestroyAllResource( Render::RenderDevice* pDevice );

    private:

        THashMap<RHI::RHIBufferCreateDesc, TVector<RHI::RHIBuffer*>>        m_bufferCache;
        THashMap<RHI::RHITextureCreateDesc, TVector<RHI::RHITexture*>>      m_textureCache;

        THashMap<String, RHI::RHIBuffer*>                                   m_namedBuffers;
        THashMap<String, RHI::RHITexture*>                                  m_namedTextures;
    };
}