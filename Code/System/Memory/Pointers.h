#pragma once

#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>

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

    template<typename T> using TSharedPtr = eastl::shared_ptr<T>;
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
}