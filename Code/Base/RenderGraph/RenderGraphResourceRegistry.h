#pragma once

#include "RenderGraphResource.h"
#include "RenderGraphNode.h"
#include "Base/Types/Arrays.h"
#include "Base/Render/RenderPipelineRegistry.h"

#include <type_traits>

namespace EE::RHI
{
    class RHIBuffer;
    class RHITexture;
    class RHIDevice;
}

namespace EE::RG
{
    template <typename Tag>
    class RGResourceHandle
    {
        friend class RenderGraph;
        friend class RGResourceRegistry;
        friend class RGNodeBuilder;

        EE_STATIC_ASSERT( ( std::is_base_of<RGResourceTagTypeBase<Tag>, Tag>::value ), "Invalid render graph resource tag!" );

    public:

        typedef typename Tag::DescType DescType;

    public:

        inline DescType const& GetDesc() const
        {
            return m_desc;
        }

    private:

        inline void Expire()
        {
            m_slotID.Expire();
        }

    private:

        DescType						m_desc;
        _Impl::RGResourceID			    m_slotID;
    };

    class RGResourceRegistry
    {
    public:

        RGResourceRegistry() = default;
        ~RGResourceRegistry() = default;

        RGResourceRegistry( RGResourceRegistry const& ) = delete;
        RGResourceRegistry& operator=( RGResourceRegistry const& ) = delete;

        RGResourceRegistry( RGResourceRegistry&& ) = default;
        RGResourceRegistry& operator=( RGResourceRegistry&& ) = default;

    public:

        enum class ResourceState
        {
            Registering = 0,
            Compiled,
            Retire,
        };
        
    public:

        inline void AttachToPipelineRegistry( Render::PipelineRegistry& pipelineRegistry ) { m_pRenderPipelineRegistry = &pipelineRegistry; }
        inline Render::PipelineRegistry* GetPipelineRegistry() const { return m_pRenderPipelineRegistry; }

        inline ResourceState GetCurrentResourceState() const { return m_resourceState; }

        // Compile all registered resources into executable resources.
        // Use RGDescType to fetch internal RHIResourceDesc and create actual RHIResource.
        // Created transient rhi resources will be cached in transient resource cache.
        void Compile( RHI::RHIDevice* pDevice );

        // Clear and release all resources.
        void ClearAll( RHI::RHIDevice* pDevice );

        //-------------------------------------------------------------------------

        template <typename RGDescType, typename RGDescCVType = typename std::add_lvalue_reference_t<std::add_const_t<RGDescType>>>
        _Impl::RGResourceID ImportResource( RGDescCVType rgDesc, RGImportedResource importResource );

        //-------------------------------------------------------------------------

        template <typename RGDescType, typename RGDescCVType = typename std::add_lvalue_reference_t<std::add_const_t<RGDescType>>>
        _Impl::RGResourceID RegisterResource( RGDescCVType rgDesc );

        [[nodiscard]] inline Render::PipelineHandle RegisterRasterPipeline( RHI::RHIRasterPipelineStateCreateDesc const& rasterPipelineDesc ) const
        {
            EE_ASSERT( m_pRenderPipelineRegistry );
            return m_pRenderPipelineRegistry->RegisterRasterPipeline( rasterPipelineDesc );
        }

        //-------------------------------------------------------------------------

        inline RGResource& GetRegisteredResource( _Impl::RGResourceID const& id )
        {
            EE_ASSERT( m_resourceState == ResourceState::Registering );
            EE_ASSERT( id.IsValid() && id.m_id < m_registeredResources.size() );
            return m_registeredResources[id.m_id];
        }

        inline RGResource const& GetRegisteredResource( _Impl::RGResourceID const& id ) const
        {
            EE_ASSERT( m_resourceState == ResourceState::Registering );
            EE_ASSERT( id.IsValid() && id.m_id < m_registeredResources.size() );
            return m_registeredResources[id.m_id];
        }

        inline RGResource& GetRegisteredResource( RGNodeResource const& nodeResource )
        {
            return GetRegisteredResource( nodeResource.m_slotID );
        }

        inline RGResource const& GetRegisteredResource( RGNodeResource const& nodeResource ) const
        {
            return GetRegisteredResource( nodeResource.m_slotID );
        }

        inline RGCompiledResource& GetCompiledResource( _Impl::RGResourceID const& id )
        {
            EE_ASSERT( m_resourceState == ResourceState::Compiled );
            EE_ASSERT( id.IsValid() && id.m_id < m_compiledResources.size() );
            return m_compiledResources[id.m_id];
        }

        inline RGCompiledResource const& GetCompiledResource( _Impl::RGResourceID const& id ) const
        {
            EE_ASSERT( m_resourceState == ResourceState::Compiled );
            EE_ASSERT( id.IsValid() && id.m_id < m_compiledResources.size() );
            return m_compiledResources[id.m_id];
        }

        inline RGCompiledResource& GetCompiledResource( RGNodeResource const& nodeResource )
        {
            return GetCompiledResource( nodeResource.m_slotID );
        }

        inline RGCompiledResource const& GetCompiledResource( RGNodeResource const& nodeResource ) const
        {
            return GetCompiledResource( nodeResource.m_slotID );
        }

        template <RGResourceViewType View>
        RHI::RHIBuffer* GetCompiledBufferResource( RGNodeResourceRef<RGResourceTagBuffer, View> const& nodeResourceRef ) const;

        template <RGResourceViewType View>
        RHI::RHITexture* GetCompiledTextureResource( RGNodeResourceRef<RGResourceTagTexture, View> const& nodeResourceRef ) const;

    private:

        ResourceState                           m_resourceState = ResourceState::Registering;

        Render::PipelineRegistry*               m_pRenderPipelineRegistry = nullptr;

        TVector<RGResource>						m_registeredResources;
        TVector<RGCompiledResource>             m_compiledResources;
    };

    template <typename RGDescType, typename RGDescCVType>
    _Impl::RGResourceID RGResourceRegistry::ImportResource( RGDescCVType rgDesc, RGImportedResource importResource )
    {
        size_t slotID = m_registeredResources.size();
        EE_ASSERT( slotID >= 0 && slotID < std::numeric_limits<uint32_t>::max() );

        // check the type of import resource matches the description
        if constexpr ( std::is_base_of<decltype( importResource.m_pImportedResource ), RHI::RHIBuffer>::value )
        {
            EE_STATIC_ASSERT( ( std::is_same<typename RGDescType, RGBufferDesc>::value ), "Expect the import resource to be the same rhi resource type as the render graph resource description!" );
        }

        if constexpr ( std::is_base_of<decltype( importResource.m_pImportedResource ), RHI::RHITexture>::value )
        {
            EE_STATIC_ASSERT( ( std::is_same<typename RGDescType, RGTextureDesc>::value ), "Expect the import resource to be the same rhi resource type as the render graph resource description!" );
        }

        _Impl::RGResourceID id( static_cast<uint32_t>( slotID ) );
        m_registeredResources.emplace_back( rgDesc, std::move( importResource ) );
        return id;
    }

    template <typename RGDescType, typename RGDescCVType>
    _Impl::RGResourceID RGResourceRegistry::RegisterResource( RGDescCVType rgDesc )
    {
        size_t slotID = m_registeredResources.size();
        EE_ASSERT( slotID >= 0 && slotID < std::numeric_limits<uint32_t>::max() );

        _Impl::RGResourceID id( static_cast<uint32_t>( slotID ) );
        m_registeredResources.emplace_back( rgDesc );
        return id;
    }

    //-------------------------------------------------------------------------

    template <RGResourceViewType View>
    RHI::RHIBuffer* RGResourceRegistry::GetCompiledBufferResource( RGNodeResourceRef<RGResourceTagBuffer, View> const& nodeResourceRef ) const
    {
        if ( m_resourceState == ResourceState::Compiled )
        {
            return m_compiledResources[nodeResourceRef.m_slotID.m_id].GetResource<RGResourceTagBuffer>();
        }

        EE_LOG_WARNING( "RenderGraph", "", "Try to fetch compiled resource but resources are not in compiled state!" );
        return nullptr;
    }

    template <RGResourceViewType View>
    RHI::RHITexture* RGResourceRegistry::GetCompiledTextureResource( RGNodeResourceRef<RGResourceTagTexture, View> const& nodeResourceRef ) const
    {
        if ( m_resourceState == ResourceState::Compiled )
        {
            return m_compiledResources[nodeResourceRef.m_slotID.m_id].GetResource<RGResourceTagTexture>();
        }

        EE_LOG_WARNING( "RenderGraph", "", "Try to fetch compiled resource but resources are not in compiled state!" );
        return nullptr;
    }
}