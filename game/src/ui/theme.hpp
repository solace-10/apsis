#pragma once

#include <imgui/imgui.hpp>

namespace WingsOfSteel::UI
{

class Theme
{
public:
    static constexpr ImColor AccentColor          = ImColor(0.02f, 0.98f, 0.75f, 1.00f);
    static constexpr ImColor ButtonHovered        = ImColor(0.02f, 0.98f, 0.75f, 0.30f);
    static constexpr ImColor ButtonText           = ImColor(0.02f, 0.98f, 0.75f, 0.75f);
    static constexpr ImColor ButtonTextHovered    = ImColor(0.02f, 0.98f, 0.75f, 1.00f);
    static constexpr ImColor DieNegativeColor     = ImColor(1.00f, 0.00f, 0.00f, 0.80f);
    static constexpr ImColor DieNeutralColor      = ImColor(1.00f, 0.50f, 0.00f, 0.80f);
    static constexpr ImColor DiePositiveColor     = ImColor(0.02f, 0.98f, 0.75f, 0.80f);
};

} // namespace WingsOfSteel::UI