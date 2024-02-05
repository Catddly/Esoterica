#include "RHIRenderPass.h"
#include "Base/Logging/Log.h"

namespace EE::RHI
{
    RHIFramebufferCache::~RHIFramebufferCache()
    {
        if ( m_bIsInitialized && !m_cachedFrameBuffers.empty() )
        {
            EE_LOG_ERROR( "RHI", "RHIFramebufferCache", "Did you forget to call ClearUp() before you destroy this framebuffer cache?" );
            EE_ASSERT( false );
        }
    }

    //-------------------------------------------------------------------------

    bool RHIFramebufferCache::Initialize( RHIRenderPass* pRenderPass, RHIRenderPassCreateDesc const& createDesc )
    {
        if ( !m_bIsInitialized && pRenderPass != nullptr )
        {
            EE_ASSERT( m_pRenderPass == nullptr );
            m_pRenderPass = pRenderPass;

            EE_ASSERT( createDesc.IsValid() );
            for ( auto const& colorAttachment : createDesc.m_colorAttachments )
            {
                m_attachmentDescs.push_back( colorAttachment );
            }

            m_colorAttachmentCount = static_cast<uint32_t>( m_attachmentDescs.size() );

            if ( createDesc.m_depthAttachment.has_value() )
            {
                m_attachmentDescs.push_back( createDesc.m_depthAttachment.value() );
            }

            m_bIsInitialized = true;
            return true;
        }

        return false;
    }

    void RHIFramebufferCache::ClearUp( RHIDevice* pDevice )
    {
        if ( pDevice != nullptr && m_bIsInitialized )
        {
            for ( auto& framebuffer : m_cachedFrameBuffers )
            {
                DestroyFramebuffer( pDevice, framebuffer.second );
            }

            m_cachedFrameBuffers.clear();
            m_attachmentDescs.clear();
            m_pRenderPass = nullptr;
            m_bIsInitialized = false;
        }
        else
        {
            EE_LOG_WARNING( "Render", "RHIFrameBufferCache::ClearUp", "Trying to call ClearUp() on uninitialized RHIFrameBufferCache!" );
        }
    }

    RHIFramebuffer* RHIFramebufferCache::GetOrCreate( RHIDevice* pDevice, RHIFramebufferCacheKey const& key )
    {
        EE_ASSERT( m_bIsInitialized );
        EE_ASSERT( m_attachmentDescs.size() == key.m_attachmentHashs.size() );
        EE_ASSERT( key.m_extentX != 0 && key.m_extentY != 0 );

        auto iter = m_cachedFrameBuffers.find( key );
        if ( iter != m_cachedFrameBuffers.end() )
        {
            return iter->second;
        }

        auto* pFramebuffer = CreateFramebuffer( pDevice, key );
        if ( pFramebuffer )
        {
            m_cachedFrameBuffers.insert( { key, pFramebuffer } );
            return pFramebuffer;
        }

        return nullptr;
    }
}