#pragma once

#include <imgui/imgui.hpp>

#include "encounter_blueprint_types.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void PinIcon(const ImVec2& size, PinIconType type, bool filled, const ImVec4& color = ImVec4(1, 1, 1, 1), const ImVec4& innerColor = ImVec4(0, 0, 0, 0));

} // namespace WingsOfSteel::TheBrightestStar
