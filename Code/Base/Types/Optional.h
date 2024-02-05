#pragma once

#include "EASTL/optional.h"

namespace EE
{
    template <typename T> using TOptional = eastl::optional<T>;
}