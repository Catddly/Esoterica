#pragma once

#include "Base/Esoterica.h"

#pragma warning (push)
#pragma warning (disable:5267) // definition of implicit copy constructor
#include <EASTL/variant.h>
#pragma warning (pop)

#include <EASTL/type_traits.h>


namespace EE
{
	template <typename... Types> using TVariant = eastl::variant<Types ...>;

    template<typename TVariant, typename T, std::size_t Index = 0>
    constexpr std::size_t GetVariantTypeIndex()
    {
        // TODO: concept check

        if constexpr ( Index == eastl::variant_size_v<TVariant> )
        {
            return Index;
        }
        else if constexpr ( eastl::is_same_v<eastl::variant_alternative_t<Index, TVariant>, T> )
        {
            return Index;
        }
        else
        {
            // compile time for-loop until Index == eastl::variant_size_v<TVariant>
            return GetVariantTypeIndex<TVariant, T, Index + 1>();
        }
    }
}