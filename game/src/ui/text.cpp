#include <imgui/imgui.hpp>

#include "ui/text.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

ElementType Text::GetType() const
{
    return ElementType::Text;
}

const std::string& Text::GetIcon() const
{
    static const std::string icon(ICON_FA_FONT);
    return icon;
}

void Text::Render()
{
    if (m_Text.empty())
    {
        return;
    }

    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + GetCellSize();

    ImGui::TextWrapped(m_Text.c_str());

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Text::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "");
    SetText(text);
}

void Text::SetText(const std::string& text)
{
    m_Text = text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI