#pragma once

#include "RenderGraphResource.h"
#include "Base/Logging/Log.h"

#include <type_traits>

namespace EE::RG
{
    template <typename Tag, RGResourceViewType RVT>
    class RGNodeResourceRef
    {
        friend class RGResourceRegistry;

        static_assert( std::is_base_of<RGResourceTagTypeBase<Tag>, Tag>::value, "Invalid render graph resource tag!" );

    public:

        RGNodeResourceRef() = default;
        RGNodeResourceRef( _Impl::RGResourceID slotID );

        auto Bind() const;

    private:

        _Impl::RGResourceID				    m_slotID;
    };

    //-------------------------------------------------------------------------

    template<typename Tag, RGResourceViewType RVT>
    RGNodeResourceRef<Tag, RVT>::RGNodeResourceRef( _Impl::RGResourceID slotID )
        : m_slotID( slotID )
    {
    }

    //-------------------------------------------------------------------------

    template <typename Tag, RGResourceViewType RVT>
    auto RGNodeResourceRef<Tag, RVT>::Bind() const
    {
        if constexpr ( std::is_same<Tag, RGResourceTagBuffer>::value && RVT == RGResourceViewType::SRV )
        {
            return RGPipelineBufferBinding{ m_slotID };
        }
        else if constexpr ( std::is_same<Tag, RGResourceTagBuffer>::value && RVT == RGResourceViewType::UAV )
        {
            return RGPipelineBufferBinding{ m_slotID };
        }
        else if constexpr ( std::is_same<Tag, RGResourceTagBuffer>::value && RVT == RGResourceViewType::RT )
        {
            EE_LOG_ERROR( "RenderGraph", "", "Invalid buffer view type combination. Buffer can NOT bind with RT view type.");
            return RGPipelineUnknownBinding{};
        }
        else if constexpr ( std::is_same<Tag, RGResourceTagTexture>::value && RVT == RGResourceViewType::SRV )
        {
            return RGPipelineTextureBinding{ RHI::RHITextureViewCreateDesc{}, m_slotID, RHI::ETextureLayout::ShaderReadOnlyOptimal };
        }
        else if constexpr ( std::is_same<Tag, RGResourceTagTexture>::value && RVT == RGResourceViewType::UAV )
        {
            return RGPipelineTextureBinding{ RHI::RHITextureViewCreateDesc{}, m_slotID, RHI::ETextureLayout::General };
        }
        else if constexpr ( std::is_same<Tag, RGResourceTagTexture>::value && RVT == RGResourceViewType::RT )
        {
            EE_LOG_ERROR( "RenderGraph", "", "Invalid texture view type combination. Texture should be bound inside renderpass." );
            return RGPipelineUnknownBinding{};
        }
    }

    // helper functions
    //-------------------------------------------------------------------------

    template <typename Tag, RGResourceViewType RVT>
    auto Bind( RGNodeResourceRef<Tag, RVT> const& nodeRef )
    {
        return nodeRef.Bind();
    }

    inline RGPipelineRHIRawBinding BindRaw( RHI::RHIPipelineResourceBinding rhiBinding )
    {
        return RGPipelineRHIRawBinding{ RHI::RHIPipelineBinding{ rhiBinding } };
    }

    inline RGPipelineStaticSamplerBinding BindStaticSampler()
    {
        return RGPipelineStaticSamplerBinding{};
    }
}