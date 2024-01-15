#include "ResourceLoader_RenderTexture.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"
#include "Base/RHI/Resource/RHITexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool TextureLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        Texture* pTextureResource = nullptr;

        if ( resID.GetResourceTypeID() == Texture::GetStaticResourceTypeID() )
        {
            pTextureResource = EE::New<Texture>();
            archive << *pTextureResource;
        }
        else if ( resID.GetResourceTypeID() == CubemapTexture::GetStaticResourceTypeID() )
        {
            auto pCubemapTextureResource = EE::New<CubemapTexture>();
            archive << *pCubemapTextureResource;
            EE_ASSERT( pCubemapTextureResource->m_format == TextureFormat::DDS );
            pTextureResource = pCubemapTextureResource;
        }
        else // Unknown type
        {
            EE_UNREACHABLE_CODE();
        }

        //-------------------------------------------------------------------------

        if ( pTextureResource->m_rawData.empty() )
        {
            EE_LOG_ERROR( "Render", "Texture Loader", "Failed to load texture resource: %s, compiled resource has no data", resID.ToString().c_str());
            return false;
        }

        pResourceRecord->SetResourceData( pTextureResource );
        return true;
    }

    Resource::InstallResult TextureLoader::Install( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord, Resource::InstallDependencyList const& installDependencies ) const
    {
        auto* pTextureResource = pResourceRecord->GetResourceData<Texture>();
        
        m_pRenderDevice->LockDevice();

        RHI::RHITextureBufferData bufferData;
        bufferData.m_binary = eastl::move( pTextureResource->m_rawData );
        bufferData.m_textureWidth = pTextureResource->GetDimensions().m_x;
        bufferData.m_textureHeight = pTextureResource->GetDimensions().m_y;
        bufferData.m_textureDepth = 1;
        bufferData.m_pixelByteLength = 4;

        RHI::RHITextureCreateDesc texDesc;
        texDesc.NewInitData( bufferData, RHI::EPixelFormat::BGRA8Unorm );

        m_pRenderDevice->GetRHIDevice()->CreateTexture( texDesc );

        //m_pRenderDevice->CreateDataTexture( *pTextureResource, pTextureResource->m_format, pTextureResource->m_rawData );
        m_pRenderDevice->UnlockDevice();

        ResourceLoader::Install( resourceID, pResourceRecord, installDependencies );
        return Resource::InstallResult::Succeeded;
    }

    void TextureLoader::Uninstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        auto* pTextureResource = pResourceRecord->GetResourceData<Texture>();
        if ( pTextureResource != nullptr && pTextureResource->IsValid() )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->GetRHIDevice()->DestroyTexture( pTextureResource->GetRHITexture() );
            m_pRenderDevice->UnlockDevice();
        }
    }

    Resource::InstallResult TextureLoader::UpdateInstall( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const
    {
        EE_UNIMPLEMENTED_FUNCTION();
        return Resource::InstallResult::Failed;
    }
}