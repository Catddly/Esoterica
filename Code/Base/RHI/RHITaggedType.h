#pragma once

#include "Base/_Module/API.h"

#include <limits>
#include <stdint.h>

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
            : m_dynamicRHIType( dynamicRhiType )
        {}
        virtual ~RHITaggedType() = default;

        inline ERHIType GetDynamicRHIType() const { return m_dynamicRHIType; }

    protected:

        ERHIType                m_dynamicRHIType = ERHIType::Invalid;
    };
}