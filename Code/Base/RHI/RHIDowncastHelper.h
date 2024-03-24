#pragma once

#include "Base/Esoterica.h"
#include "Base/Memory/Pointers.h"
#include "RHITaggedType.h"

#include <type_traits>

namespace EE::RHI
{
    template <typename TPointerType>
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

        TPointerType&       operator*()       { EE_ASSERT( m_pPointer ); return *m_pPointer; }
        TPointerType const& operator*() const { EE_ASSERT( m_pPointer ); return *m_pPointer; }

        TPointerType*       operator->()       { EE_ASSERT( m_pPointer ); return m_pPointer; }
        TPointerType const* operator->() const { EE_ASSERT( m_pPointer ); return m_pPointer; }

        operator bool() const { return IsValid(); }

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return m_pPointer != nullptr; }

    private:

        template <typename To, typename FromInnerType>
        friend RHIDowncastRawPointerGuard<To> RHIDowncast( TTSSharedPtr<FromInnerType>& rhiRef );

        RHIDowncastRawPointerGuard( TPointerType* pPointer )
            : m_pPointer( pPointer )
        {
        }

    private:

        TPointerType*                   m_pPointer = nullptr;
    };

    template <typename TPointerType>
    RHIDowncastRawPointerGuard<TPointerType>::~RHIDowncastRawPointerGuard()
    {
        m_pPointer = nullptr;
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
    RHIDowncastRawPointerGuard<To> RHIDowncast( TTSSharedPtr<FromInnerType>& rhiRef )
    {
        FromInnerType* pInnerPtr = rhiRef.get();
        To* pConverted = RHIDowncast<To, FromInnerType>( pInnerPtr );
        return RHIDowncastRawPointerGuard( pConverted );
    }
}