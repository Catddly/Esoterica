#include "RenderGraphTransientResourceCache.h"
#include "Base/Esoterica.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "Base/RHI/Resource/RHITexture.h"

namespace EE::RG
{
    RHI::RHIBuffer* RGTransientResourceCache::FetchAvailableTemporaryBuffer( RHI::RHIBufferCreateDesc const& bufferDesc )
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

    RHI::RHITexture* RGTransientResourceCache::FetchAvailableTemporaryTexture( RHI::RHITextureCreateDesc const& textureDesc )
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

    bool RGTransientResourceCache::UpdateDirtyNamedBuffer( String const& name, RHI::RHIDevice* pDevice, RHI::RHIBufferCreateDesc const& bufferDesc )
    {
        auto iterator = m_namedBuffers.find( name );
        if ( iterator == m_namedBuffers.end() )
        {
            return false;
        }

        auto* pStaleBuffer = iterator->second;
        EE_ASSERT( pStaleBuffer );

        if ( pStaleBuffer->GetDesc() != bufferDesc )
        {
            // update stale buffer immediately
            pDevice->DeferRelease( pStaleBuffer );
            auto* pNewBuffer = pDevice->CreateBuffer( bufferDesc );
            EE_ASSERT( pNewBuffer );

            m_namedBuffers[name] = pNewBuffer;

            return true;
        }

        return false;
    }

    bool RGTransientResourceCache::UpdateDirtyNamedTexture( String const& name, RHI::RHIDevice* pDevice, RHI::RHITextureCreateDesc const& textureDesc )
    {
        auto iterator = m_namedTextures.find( name );
        if ( iterator == m_namedTextures.end() )
        {
            return false;
        }

        auto* pStaleTexture = iterator->second;
        EE_ASSERT( pStaleTexture );

        if ( pStaleTexture->GetDesc() != textureDesc )
        {
            // update stale texture immediately
            pDevice->DeferRelease( pStaleTexture );
            auto* pNewTexture = pDevice->CreateTexture( textureDesc );
            EE_ASSERT( pNewTexture );

            m_namedTextures[name] = pNewTexture;

            return true;
        }

        return false;
    }

    RHI::RHIBuffer* RGTransientResourceCache::GetOrCreateNamedBuffer( String const& name, RHI::RHIDevice* pDevice, RHI::RHIBufferCreateDesc const& bufferDesc )
    {
        auto iterator = m_namedBuffers.find( name );
        if ( iterator != m_namedBuffers.end() )
        {
            return iterator->second;
        }
           
        auto* pNewBuffer = pDevice->CreateBuffer( bufferDesc );
        EE_ASSERT( pNewBuffer );
        m_namedBuffers.insert( { name, pNewBuffer } );

        return pNewBuffer;
    }

    RHI::RHITexture* RGTransientResourceCache::GetOrCreateNamedTexture( String const& name, RHI::RHIDevice* pDevice, RHI::RHITextureCreateDesc const& textureDesc )
    {
        auto iterator = m_namedTextures.find( name );
        if ( iterator != m_namedTextures.end() )
        {
            return iterator->second;
        }

        auto* pNewTexture = pDevice->CreateTexture( textureDesc );
        EE_ASSERT( pNewTexture );
        m_namedTextures.insert( { name, pNewTexture } );

        return pNewTexture;
    }

    //-------------------------------------------------------------------------

    void RGTransientResourceCache::RestoreBuffer( RHI::RHIBuffer* pBuffer )
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

    void RGTransientResourceCache::RestoreTexture( RHI::RHITexture* pTexture )
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

    //-------------------------------------------------------------------------

    void RGTransientResourceCache::DestroyAllResource( RHI::RHIDevice* pDevice )
    {
        EE_ASSERT( pDevice != nullptr );

        for ( auto& [name, namedBuffer] : m_namedBuffers )
        {
            pDevice->DestroyBuffer( namedBuffer );
            namedBuffer = nullptr;
        }

        for ( auto& [name, namedTexture] : m_namedTextures )
        {
            pDevice->DestroyTexture( namedTexture );
            namedTexture = nullptr;
        }

        m_namedBuffers.clear();
        m_namedTextures.clear();

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