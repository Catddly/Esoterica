#pragma once

#include "Base/Esoterica.h"
#include "Base/Memory/Pointers.h"
#include "RHITaggedType.h"

#include <type_traits>

namespace EE::RHI
{
    template <typename T, typename TRaw>
    class RHIDowncastRawPointerGuard
    {
    public:

        RHIDowncastRawPointerGuard( RHIDowncastRawPointerGuard const& rhs ) = delete;
        RHIDowncastRawPointerGuard& operator=( RHIDowncastRawPointerGuard const& rhs ) = delete;

        RHIDowncastRawPointerGuard( RHIDowncastRawPointerGuard&& rhs ) = delete;
        RHIDowncastRawPointerGuard& operator=( RHIDowncastRawPointerGuard&& rhs ) = delete;

        //-------------------------------------------------------------------------

        ~RHIDowncastRawPointerGuard();

        //-------------------------------------------------------------------------

        TRaw&       operator*()       { EE_ASSERT( !m_pWeak.expired() ); return m_pRawConverted; }
        TRaw const& operator*() const { EE_ASSERT( !m_pWeak.expired() ); return m_pRawConverted; }

        TRaw*       operator->()       { EE_ASSERT( !m_pWeak.expired() ); return m_pRawConverted; }
        TRaw const* operator->() const { EE_ASSERT( !m_pWeak.expired() ); return m_pRawConverted; }

        operator bool() const { return IsValid(); }

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return !m_pWeak.expired() && m_pRawConverted; }

    private:

        template <typename To, typename FromInnerType>
        friend RHIDowncastRawPointerGuard<To> RHIDowncast( TSharedPtr<FromInnerType>& rhiRef );

        RHIDowncastRawPointerGuard( TRaw* pRaw, TSharedPtr<T> const& pPointer )
            : m_pRawConverted( pRaw ), m_pWeak( pPointer )
        {
        }

    private:

        TRaw*                         m_pRawConverted = nullptr;
        TWeakPtr<T>                   m_pWeak = nullptr;
    };

    template <typename T, typename TRaw>
    RHIDowncastRawPointerGuard<T, TRaw>::~RHIDowncastRawPointerGuard()
    {
        m_pRawConverted = nullptr;
    }

    //-------------------------------------------------------------------------

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

    // RHI Reference Type
    //-------------------------------------------------------------------------

    template <typename To, typename FromInnerType>
    RHIDowncastRawPointerGuard<FromInnerType, To> RHIDowncast( TSharedPtr<FromInnerType>& rhiRef )
    {
        FromInnerType* pInnerPtr = rhiRef.get();
        To* pConverted = RHIDowncast<To, FromInnerType>( pInnerPtr );
        return RHIDowncastRawPointerGuard( pConverted, rhiRef );
    }

    template <typename To, typename FromInnerType>
    RHIDowncastRawPointerGuard<FromInnerType, To const> RHIDowncast( TSharedPtr<FromInnerType> const& rhiRef )
    {
        FromInnerType const* pInnerPtr = rhiRef.get();
        To const* pConverted = RHIDowncast<To const, FromInnerType const>( pInnerPtr );
        return RHIDowncastRawPointerGuard( pConverted, rhiRef );
    }
}