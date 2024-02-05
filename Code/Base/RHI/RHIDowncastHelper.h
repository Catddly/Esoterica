#pragma once

#include "Base/Esoterica.h"
#include "RHITaggedType.h"

#include <type_traits>

namespace EE::RHI
{
    template <typename To, typename From>
    To* RHIDowncast( From* pRHI )
    {
        EE_STATIC_ASSERT( ( std::is_base_of<RHITaggedType, From>::value ), "Try to downcast a non-decorated rhi type!" );

        if ( !pRHI )
        {
            return nullptr;
        }

        EE_ASSERT( pRHI->GetDynamicRHIType() != ERHIType::Invalid );

        if ( pRHI->GetDynamicRHIType() == To::GetStaticRHIType() )
        {
            // Safety: we manually ensure that T and U shared the same rhi,
            //         and it has a valid derived relationship.
            //         This downcast is safe.
            return static_cast<To*>( pRHI );
        }

        // TODO: may be use EE::IReflectedType?
        EE_LOG_ERROR( "RHI", "RHI::Downcast", "Try to downcast to irrelevant rhi type or invalid derived class!" );
        return nullptr;
    }

    template <typename To, typename From>
    To const* RHIDowncast( From const* pRHI )
    {
        EE_STATIC_ASSERT( ( std::is_base_of<RHITaggedType, From>::value ), "Try to downcast a non-decorated rhi type!" );

        if ( !pRHI )
        {
            return nullptr;
        }

        EE_ASSERT( pRHI->GetDynamicRHIType() != ERHIType::Invalid );

        if ( pRHI->GetDynamicRHIType() == To::GetStaticRHIType() )
        {
            // Safety: we manually ensure that T and U shared the same rhi,
            //         and it has a valid derived relationship.
            //         This downcast is safe.
            return static_cast<To const*>( pRHI );
        }

        // TODO: may be use EE::IReflectedType?
        EE_LOG_ERROR( "RHI", "RHI::Downcast", "Try to downcast to irrelevant rhi type or invalid derived class!" );
        return nullptr;
    }
}