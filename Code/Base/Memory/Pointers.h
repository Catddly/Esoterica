#pragma once

#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <eathread/shared_ptr_mt.h>

//-------------------------------------------------------------------------

namespace EE
{
    namespace Memory
    {
        template <typename T>
        struct DefaultDeleter
        {
            EA_CONSTEXPR DefaultDeleter() EA_NOEXCEPT = default;

            void operator()( T* p ) const EA_NOEXCEPT
            {
                EE::Delete( p );
            }
        };

        template <typename T>
        struct DefaultDeleter<T[]> // Specialization for arrays.
        {
            EA_CONSTEXPR DefaultDeleter() EA_NOEXCEPT = default;

            void operator()( T* p ) const EA_NOEXCEPT
            {
                EE::DeleteArray( p );
            }
        };
    }

    //-------------------------------------------------------------------------

    template<typename T, typename D = Memory::DefaultDeleter<T>> using TSharedPtr = eastl::shared_ptr<T>;
    template<typename T, typename D = Memory::DefaultDeleter<T>> using TUniquePtr = eastl::unique_ptr<T, D>;

    template <typename T, typename... Args>
    TSharedPtr<T> MakeShared( Args&&... args )
    {
        return eastl::make_shared<T>( eastl::forward<Args>( args )... );
    }

    template <typename T, typename... Args>
    TUniquePtr<T> MakeUnique( Args&&... args )
    {
        return eastl::make_unique<T>( eastl::forward<Args>( args )... );
    }

    //-------------------------------------------------------------------------

    // TS stands for thread safe
    template <typename T, typename D = Memory::DefaultDeleter<T>> using TTSSharedPtr = EA::Thread::shared_ptr_mt<T>;

    template <typename T, typename... Args>
    TTSSharedPtr<T> MakeTSShared( Args&&... args )
    {
        return TTSSharedPtr<T>( EE::New<T>( eastl::forward<Args>( args )... ) );
    }

    template <typename T, typename D, typename... Args>
    TTSSharedPtr<D> MakeTSShared( Args&&... args )
    {
        return TTSSharedPtr<D>( EE::New<T>( eastl::forward<Args>( args )... ) );
    }
}

namespace EE::Trait
{
    // helper type traits
    template <typename T>
    struct IsSmartPointer
    {
        static constexpr bool value = false;
    };

    template <typename U>
    struct IsSmartPointer<TSharedPtr<U>>
    {
        static constexpr bool value = true;
    };

    template <typename U>
    struct IsSmartPointer<TUniquePtr<U>>
    {
        static constexpr bool value = true;
    };

    namespace _Impl
    {
        template <typename T>
        struct IsPointerIncludeSmartPointerImpl
        {
            static constexpr bool value = eastl::disjunction<eastl::is_pointer<T>, IsSmartPointer<T>>::value;
        };
    }

    template <typename T>
    struct IsPointerIncludeSmartPointer : std::bool_constant<_Impl::IsPointerIncludeSmartPointerImpl<T>::value>
    {};
}