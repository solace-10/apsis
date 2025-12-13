#pragma once

#include <imgui/imgui.hpp>

namespace WingsOfSteel::UI
{

class Theme
{
public:
    static constexpr ImColor AccentColor          = ImColor(0.94f, 0.21f, 0.33f, 1.00f);
    static constexpr ImColor ButtonHovered        = ImColor(0.94f, 0.21f, 0.33f, 0.30f);
    static constexpr ImColor ButtonText           = ImColor(1.00f, 0.45f, 0.55f, 1.00f);
    static constexpr ImColor ButtonTextHovered    = ImColor(1.00f, 0.65f, 0.72f, 1.00f);
};

} // namespace WingsOfSteel::UI
