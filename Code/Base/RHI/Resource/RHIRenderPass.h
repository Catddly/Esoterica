#pragma once

#include "Base/Types/HashMap.h"
#include "../RHITaggedType.h"
#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIDevice;
    class RHIRenderPass;

    class RHIFramebuffer : public RHIResource
    {
    public:

        RHIFramebuffer( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHIFramebuffer() = default;
    };

    class RHIFramebufferCache
    {
        using RHIRenderPassAttachmentDescs = TFixedVector<RHIRenderPassAttachmentDesc, RHIRenderPassCreateDesc::NumMaxAttachmentCount>;

    public:

        RHIFramebufferCache() = default;
        virtual ~RHIFramebufferCache();

        RHIFramebufferCache( RHIFramebufferCache const& ) = delete;
        RHIFramebufferCache& operator=( RHIFramebufferCache const& ) = delete;

        RHIFramebufferCache( RHIFramebufferCache&& ) = default;
        RHIFramebufferCache& operator=( RHIFramebufferCache&& ) = default;

    public:

        bool Initialize( RHIRenderPass* pRenderPass, RHIRenderPassCreateDesc const& createDesc );
        // After clear up, this framebuffer cache is invalid.
        // You need to call Initialize to make it valid.
        void ClearUp( RHIDevice* pDevice );

        RHIFramebuffer* GetOrCreate( RHIDevice* pDevice, RHIFramebufferCacheKey const& key );

        inline uint32_t GetColorAttachmentCount() const { return m_colorAttachmentCount; }

    protected:

        virtual RHIFramebuffer* CreateFramebuffer( RHIDevice* pDevice, RHIFramebufferCacheKey const& key ) = 0;
        virtual void            DestroyFramebuffer( RHIDevice* pDevice, RHIFramebuffer* pFramebuffer ) = 0;

    protected:

        THashMap<RHIFramebufferCacheKey, RHIFramebuffer*>   m_cachedFrameBuffers;
        RHIRenderPassAttachmentDescs                        m_attachmentDescs;
        RHIRenderPass*                                      m_pRenderPass = nullptr;
        uint32_t                                            m_colorAttachmentCount = 0;

        bool                                                m_bIsInitialized = false;
    };

    class EE_BASE_API RHIRenderPass : public RHIResource
    {
    public:

        RHIRenderPass( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHIRenderPass() = default;

        inline RHIFramebuffer* GetOrCreateFramebuffer( RHIDevice* pDevice, RHIFramebufferCacheKey const& key )
        {
            EE_ASSERT( m_pFramebufferCache );
            return m_pFramebufferCache->GetOrCreate( pDevice, key );
        }

    protected:

        RHIFramebufferCache*                m_pFramebufferCache = nullptr;
        RHIRenderPassCreateDesc             m_desc;
    };
}