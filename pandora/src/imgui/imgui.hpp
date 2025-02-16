#pragma once

#include <string>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "imgui/fonts/icons_font_awesome.hpp"

namespace ImGui
{
    bool Button(const std::string& label, const ImVec2& size, bool enabled);
}