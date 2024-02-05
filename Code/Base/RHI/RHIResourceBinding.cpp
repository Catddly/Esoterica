#include "RHIResourceBinding.h"
#include "Base/Encoding/Hash.h"

namespace EE::RHI
{
    size_t RHIPipelineBinding::GetHash() const
    {
        size_t hash = 0;
        Hash::HashCombine( hash, m_binding.index() );

        if ( m_binding.index() == GetVariantTypeIndex<decltype( m_binding ), RHIBufferBinding>() )
        {
            auto const& bufferBinding = eastl::get<RHIBufferBinding>( m_binding );
            Hash::HashCombine( hash, bufferBinding.m_pBuffer );
        }
        else if ( m_binding.index() == GetVariantTypeIndex<decltype( m_binding ), RHIDynamicBufferBinding>() )
        {
            auto const& dynBufferBinding = eastl::get<RHIDynamicBufferBinding>( m_binding );
            Hash::HashCombine( hash, dynBufferBinding.m_pBuffer );
            Hash::HashCombine( hash, dynBufferBinding.m_dynamicOffset );
        }
        else if ( m_binding.index() == GetVariantTypeIndex<decltype( m_binding ), RHITextureBinding>() )
        {
            auto const& textureBinding = eastl::get<RHITextureBinding>( m_binding );
            Hash::HashCombine( hash, textureBinding.m_view );
            Hash::HashCombine( hash, textureBinding.m_layout );
        }
        else if ( m_binding.index() == GetVariantTypeIndex<decltype( m_binding ), RHITextureArrayBinding>() )
        {
            auto const& textureArrayBinding = eastl::get<RHITextureArrayBinding>( m_binding );
            EE_ASSERT( !textureArrayBinding.m_bindings.empty() );

            for ( auto const& texBind : textureArrayBinding.m_bindings )
            {
                Hash::HashCombine( hash, texBind.m_view );
                Hash::HashCombine( hash, texBind.m_layout );
            }
        }
        else if ( m_binding.index() == GetVariantTypeIndex<decltype( m_binding ), RHIUnknownBinding>() )
        {
        }

        return hash;
    }
}