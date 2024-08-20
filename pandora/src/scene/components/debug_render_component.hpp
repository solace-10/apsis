#pragma once

#include <optional>

#include "core/color.hpp"

namespace WingsOfSteel::Pandora
{

enum class DebugRenderShape
{
    Circle,
    Cone,
    Box
};

class DebugRenderComponent
{
public:
    DebugRenderComponent() {}

    std::optional<float> radius;
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> length;
    Color color = Color::White;
    DebugRenderShape shape = DebugRenderShape::Cone;
};

} // namespace WingsOfSteel::Pandora