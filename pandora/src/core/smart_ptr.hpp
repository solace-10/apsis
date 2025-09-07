#pragma once

#include <memory>

namespace WingsOfSteel
{

#define DECLARE_SMART_PTR(x)                 \
    class x;                                 \
    using x##SharedPtr = std::shared_ptr<x>; \
    using x##WeakPtr = std::weak_ptr<x>;     \
    using x##UniquePtr = std::unique_ptr<x>;

} // namespace WingsOfSteel
