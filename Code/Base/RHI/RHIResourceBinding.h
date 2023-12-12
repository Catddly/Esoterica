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
    };

    struct RHIDynamicBufferBinding
    {
        RHIBuffer*                              m_pBuffer = nullptr;
        uint32_t                                m_dynamicOffset = 0;
    };

    struct RHITextureBinding
    {
        RHITextureView                          m_view;
        RHI::ETextureLayout                     m_layout;
    };

    struct RHITextureArrayBinding
    {
        TInlineVector<RHITextureBinding, 16>    m_bindings;
    };

    // Use as a placeholder
    struct RHIStaticSamplerBinding
    {
    };
    
    struct RHIUnknownBinding
    {
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

    public:

        RHIPipelineResourceBinding                   m_binding = RHIUnknownBinding{};
    };
}