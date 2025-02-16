#include "imgui/imgui.hpp"

namespace ImGui
{

bool Button(const std::string& label, const ImVec2& size, bool enabled)
{
    if (enabled)
    {
        return Button(label.c_str(), size);
    }
    else
    {
        static const ImVec4 disabledColor(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
        ImGui::Button(label.c_str(), size);
        ImGui::PopStyleColor(3);
        return false;
    }
}

} // namespace ImGui