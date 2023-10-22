#include "RenderGraphTransientResourceCache.h"
#include "Base/Esoterica.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "Base/RHI/Resource/RHITexture.h"

namespace EE::RG
{
    RHI::RHIBuffer* RGTransientResourceCache::FetchAvailableBuffer( RHI::RHIBufferCreateDesc const& bufferDesc )
    {
        auto iterator = m_bufferCache.find( bufferDesc );
        if ( iterator != m_bufferCache.end() )
        {
            auto& buffers = iterator->second;

            if ( buffers.empty() )
            {
                return nullptr;
            }

            RHI::RHIBuffer* pBuffer = buffers.back();
            buffers.pop_back();
            return pBuffer;
        }

        return nullptr;
    }

    RHI::RHITexture* RGTransientResourceCache::FetchAvailableTexture( RHI::RHITextureCreateDesc const& textureDesc )
    {
        auto iterator = m_textureCache.find( textureDesc );
        if ( iterator != m_textureCache.end() )
        {
            auto& textures = iterator->second;

            if ( textures.empty() )
            {
                return nullptr;
            }

            RHI::RHITexture* pTexture = textures.back();
            textures.pop_back();
            return pTexture;
        }

        return nullptr;
    }

    void RGTransientResourceCache::StoreBufferResource( RHI::RHIBuffer* pBuffer )
    {
        auto const desc = pBuffer->GetDesc();
        auto iterator = m_bufferCache.find( desc );
        if ( iterator == m_bufferCache.end() )
        {
            auto& buffers = m_bufferCache.insert( desc ).first->second;
            buffers.push_back( pBuffer );

            return;
        }
        
        iterator->second.push_back( pBuffer );
    }

    void RGTransientResourceCache::StoreTextureResource( RHI::RHITexture* pTexture )
    {
        auto const desc = pTexture->GetDesc();
        auto iterator = m_textureCache.find( desc );
        if ( iterator == m_textureCache.end() )
        {
            auto& textures = m_textureCache.insert( desc ).first->second;
            textures.push_back( pTexture );

            return;
        }

        iterator->second.push_back( pTexture );
    }

    void RGTransientResourceCache::DestroyAllResource( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( pDevice != nullptr );

        for ( auto& [desc, buffers] : m_bufferCache )
        {
            for ( auto& pBuffer : buffers )
            {
                pDevice->DestroyBuffer( pBuffer );
                pBuffer = nullptr;
            }
        }

        for ( auto& [desc, textures] : m_textureCache )
        {
            for ( auto& pTexture : textures )
            {
                pDevice->DestroyTexture( pTexture );
                pTexture = nullptr;
            }
        }

        m_bufferCache.clear();
        m_textureCache.clear();
    }
}