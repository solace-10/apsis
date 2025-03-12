#include <imgui/imgui.hpp>

#include "ui/text.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Text::Text()
{
    AddFlag(Flags::AutoSize);
}

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
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + glm::vec2(m_Margin);
    const ImVec2 cp1 = cp0 + GetCellSize() - glm::vec2(m_Margin * 2);

    ImGui::SetCursorScreenPos(cp0);

    if (!m_Text.empty())
    {
        //ImGui::PushTextWrapPos()
        ImGui::TextWrapped(m_Text.c_str());
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Text::RenderProperties()
{
    StackableElement::RenderProperties();
    ImGui::InputInt("Margin", &m_Margin);
    ImGui::InputTextMultiline("Text", &m_Text);
}

void Text::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "margin", m_Margin, 0);
}

nlohmann::json Text::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["text"] = m_Text;
    data["margin"] = m_Margin;
    return data;
}

void Text::SetText(const std::string& text)
{
    m_Text = text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI