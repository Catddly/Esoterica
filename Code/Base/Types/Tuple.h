#pragma once

#include "EASTL/tuple.h"

namespace EE
{
    template <typename T1, typename T2>
    using TPair = eastl::pair<T1, T2>;

    template <typename... Types>
    using TTuple = eastl::tuple<Types...>;
}