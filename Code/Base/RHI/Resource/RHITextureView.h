#pragma once

#include "RHIResourceCreationCommons.h"
#include "Base/Encoding/Hash.h"
#include "Base/Types/Variant.h"

namespace EE::RHI
{
    struct RHITextureView
    {
        inline bool IsValid() const { return m_pViewHandle != nullptr; }

        RHITextureViewCreateDesc            m_desc = {};
        void*                               m_pViewHandle = nullptr;

        friend bool operator==( RHITextureView const& lhs, RHITextureView const& rhs )
        {
            return eastl::tie( lhs.m_desc, lhs.m_pViewHandle )
                == eastl::tie( rhs.m_desc, rhs.m_pViewHandle );
        }
    };
}

// Hash
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::RHI::RHITextureView>
    {
        eastl_size_t operator()( EE::RHI::RHITextureView const& textureView ) const noexcept
        {
            eastl_size_t hash = 0;
            EE::Hash::HashCombine( hash, textureView.m_desc );
            EE::Hash::HashCombine( hash, textureView.m_pViewHandle );
            return hash;
        }
    };
}