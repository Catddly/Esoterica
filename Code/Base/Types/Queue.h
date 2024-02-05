#pragma once

#include <EASTL/queue.h>

namespace EE
{
    template <typename T>
    using TQueue = eastl::queue<T>;
}