#include <imgui/imgui.hpp>

#include "ui/internal/default_markdown.hpp"
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
    const ImVec2 contentSize = GetCellSize() - glm::vec2(m_Margin * 2);
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + glm::vec2(m_Margin);
    const ImVec2 cp1 = cp0 + contentSize;

    ImGui::SetCursorScreenPos(cp0);

    if (m_IsScrollable)
    {
        ImGui::BeginChild("Text", contentSize);
    }

    if (!m_Text.empty())
    {
        if (m_IsMarkdown)
        {
            ImGui::Markdown(m_Text.c_str(), m_Text.length(), Internal::DefaultMarkdown::Get());
        }
        else
        {
            ImGui::TextWrapped("%s", m_Text.c_str());
        }
    }

    if (m_IsScrollable)
    {
        ImGui::EndChild();
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

    ImGui::BeginDisabled(m_IsDynamic);
    ImGui::InputTextMultiline("Text", &m_Text);
    ImGui::EndDisabled();

    ImGui::Checkbox("Markdown", &m_IsMarkdown);
    ImGui::Checkbox("Scrollable", &m_IsScrollable);
    ImGui::Checkbox("Dynamic", &m_IsDynamic);
}

void Text::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "margin", m_Margin, 0);
    TryDeserialize(data, "is_markdown", m_IsMarkdown, false);
    TryDeserialize(data, "is_scrollable", m_IsScrollable, false);
    TryDeserialize(data, "is_dynamic", m_IsDynamic, false);
}

nlohmann::json Text::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();

    if (!m_IsDynamic)
    {
        data["text"] = m_Text;
    }
    data["margin"] = m_Margin;
    data["is_markdown"] = m_IsMarkdown;
    data["is_scrollable"] = m_IsScrollable;
    data["is_dynamic"] = m_IsDynamic;
    return data;
}

void Text::SetText(const std::string& text)
{
    m_Text = text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI