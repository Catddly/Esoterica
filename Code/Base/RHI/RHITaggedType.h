#pragma once

#include "Base/_Module/API.h"

#include <limits>
#include <stdint.h>

// helper macro

#define EE_RHI_STATIC_TAGGED_TYPE( rhiType ) inline static EE::RHI::ERHIType GetStaticRHIType() { return rhiType; }

//-------------------------------------------------------------------------

namespace EE::RHI
{
    enum class ERHIType : uint8_t
    {
        Vulkan = 0,
        DX11,

        Invalid = std::numeric_limits<uint8_t>::max()
    };

    class EE_BASE_API RHITaggedType
    {
    public:

        RHITaggedType( ERHIType dynamicRhiType )
            : m_DynamicRHIType( dynamicRhiType )
        {}
        virtual ~RHITaggedType() = default;

        inline ERHIType GetDynamicRHIType() const { return m_DynamicRHIType; }

    protected:

        ERHIType                m_DynamicRHIType = ERHIType::Invalid;
    };
}