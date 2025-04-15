#include <imgui/imgui.hpp>

#include "ui/heading.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Heading::Heading()
{
    AddFlag(Flags::AutoSize);
}

ElementType Heading::GetType() const
{
    return ElementType::Heading;
}

const std::string& Heading::GetIcon() const
{
    static const std::string icon(ICON_FA_HEADING);
    return icon;
}

void Heading::Render()
{
    const ImVec2 contentSize = GetCellSize();
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + contentSize;

    ImGui::SetCursorScreenPos(cp0);

    if (!m_Text.empty())
    {
        ImGui::Text("%s", m_Text.c_str());
    }

    if (m_Underlined)
    {
        static const ImU32 accentColor = IM_COL32(5, 250, 191, 255);
        const ImVec2 titleUnderlineStart(cp0 + ImVec2(0, 24));
        const ImVec2 titleUnderlineEnd(titleUnderlineStart + ImVec2(contentSize.x, 0));
        ImGui::GetWindowDrawList()->AddLine(titleUnderlineStart, titleUnderlineEnd, accentColor);
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Heading::RenderProperties()
{
    StackableElement::RenderProperties();

    ImGui::BeginDisabled(m_IsDynamic);
    ImGui::InputTextMultiline("Text", &m_Text);
    ImGui::EndDisabled();

    ImGui::Checkbox("Dynamic", &m_IsDynamic);
    ImGui::Checkbox("Underlined", &m_Underlined);
}

void Heading::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "is_dynamic", m_IsDynamic, false);
    TryDeserialize(data, "underlined", m_Underlined, true);
}

nlohmann::json Heading::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();

    if (!m_IsDynamic)
    {
        data["text"] = m_Text;
    }
    data["is_dynamic"] = m_IsDynamic;
    data["underlined"] = m_Underlined;
    return data;
}

} // namespace WingsOfSteel::TheBrightestStar::UI