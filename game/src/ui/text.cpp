#include <imgui/imgui.hpp>

#include "ui/text.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

void Text::Render()
{
    if (m_Text.empty())
    {
        return;
    }

    //ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + GetPosition());
    //ImGui::Text(m_Text.c_str());
}

void Text::SetText(const std::string& text)
{
    m_Text = text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI