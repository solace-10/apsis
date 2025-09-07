#pragma once

#include <memory>

namespace WingsOfSteel::Pandora
{

#define DECLARE_SMART_PTR(x)                 \
    class x;                                 \
    using x##SharedPtr = std::shared_ptr<x>; \
    using x##WeakPtr = std::weak_ptr<x>;     \
    using x##UniquePtr = std::unique_ptr<x>;

} // namespace WingsOfSteel::Pandora
