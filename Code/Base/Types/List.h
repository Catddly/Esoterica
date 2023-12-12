#pragma once

#include <EASTL/list.h>
#include <EASTL/fixed_list.h>
#include <EASTL/slist.h>
#include <EASTL/fixed_slist.h>

namespace EE
{
    // Bidirectional list
    template <typename E> using TList = eastl::list<E>;

    template<typename T, eastl_size_t S> using TInlineList = eastl::fixed_list<T, S, true>;
    template<typename T, eastl_size_t S> using TFixedList = eastl::fixed_list<T, S, false>;

    // Unidirectional list
    template <typename E> using TSList = eastl::slist<E>;

    template<typename T, eastl_size_t S> using TSInlineList = eastl::fixed_slist<T, S, true>;
    template<typename T, eastl_size_t S> using TSFixedList = eastl::fixed_slist<T, S, false>;
}