#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Types/Variant.h"
#include "Resource/RHITextureView.h"
#include "Resource/RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIBuffer;

    struct RHIBufferBinding
    {
        RHIBuffer*                              m_pBuffer = nullptr;

        friend bool operator==( RHIBufferBinding const& lhs, RHIBufferBinding const& rhs )
        {
            return lhs.m_pBuffer == rhs.m_pBuffer;
        }
    };

    struct RHIDynamicBufferBinding
    {
        RHIBuffer*                              m_pBuffer = nullptr;
        uint32_t                                m_dynamicOffset = 0;

        friend bool operator==( RHIDynamicBufferBinding const& lhs, RHIDynamicBufferBinding const& rhs )
        {
            return eastl::tie( lhs.m_pBuffer, lhs.m_dynamicOffset )
                == eastl::tie( rhs.m_pBuffer, rhs.m_dynamicOffset );
        }
    };

    struct RHITextureBinding
    {
        RHITextureView                          m_view;
        ETextureLayout                          m_layout;

        friend bool operator==( RHITextureBinding const& lhs, RHITextureBinding const& rhs )
        {
            return eastl::tie( lhs.m_view, lhs.m_layout )
                == eastl::tie( rhs.m_view, rhs.m_layout );
        }

        friend bool operator!=( RHITextureBinding const& lhs, RHITextureBinding const& rhs )
        {
            return !operator==( lhs, rhs );
        }
    };

    struct RHITextureArrayBinding
    {
        TInlineVector<RHITextureBinding, 16>    m_bindings;

        friend bool operator==( RHITextureArrayBinding const& lhs, RHITextureArrayBinding const& rhs )
        {
            if ( lhs.m_bindings.size() != rhs.m_bindings.size() )
            {
                return false;
            }

            for ( size_t i = 0; i < lhs.m_bindings.size(); ++i )
            {
                if ( lhs.m_bindings[i] != rhs.m_bindings[i] )
                {
                    return false;
                }
            }

            return true;
        }
    };

    // Use as a placeholder
    struct RHIStaticSamplerBinding
    {
        inline friend bool operator==( RHIStaticSamplerBinding const&, RHIStaticSamplerBinding const& )
        {
            return true;
        }
    };
    
    // Use as a placeholder
    struct RHIUnknownBinding
    {
        inline friend bool operator==( RHIUnknownBinding const&, RHIUnknownBinding const& )
        {
            return true;
        }
    };

    using RHIPipelineResourceBinding = TVariant<
        RHIBufferBinding,
        RHIDynamicBufferBinding,
        RHITextureBinding,
        RHITextureArrayBinding,
        RHIStaticSamplerBinding,
        RHIUnknownBinding
    >;

    struct RHIPipelineBinding
    {
    public:

        RHIPipelineBinding( RHIPipelineResourceBinding const& binding )
            : m_binding( binding )
        {
        }

        size_t GetHash() const;

        inline friend bool operator==( RHIPipelineBinding const& lhs, RHIPipelineBinding const& rhs )
        {
            return lhs.m_binding == rhs.m_binding;
        }

    public:

        RHIPipelineResourceBinding                   m_binding = RHIUnknownBinding{};
    };
}

// Hash
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::RHI::RHIPipelineBinding>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::RHI::RHIPipelineBinding const& rhiPipelineBinding )
        {
            return rhiPipelineBinding.GetHash();
        }
    };
}