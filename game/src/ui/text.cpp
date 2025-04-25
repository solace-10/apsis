#include <imgui/imgui.hpp>
#include <magic_enum.hpp>

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
        if (m_Mode == Mode::Markdown)
        {
            ImGui::Markdown(m_Text.c_str(), m_Text.length(), Internal::DefaultMarkdown::Get());
        }
        else if (m_Mode == Mode::MultiLine)
        {
            ImGui::TextWrapped("%s", m_Text.c_str());
        }
        else if (m_Mode == Mode::SingleLine)
        {
            ImGui::TextUnformatted(m_Text.c_str());
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

    int mode = static_cast<int>(m_Mode);
    if (ImGui::Combo("Mode", &mode, "Single line\0Multi line\0Markdown\0"))
    {
        m_Mode = static_cast<Mode>(mode);
    }

    ImGui::BeginDisabled(HasFlag(Flags::Bound));
    ImGui::InputTextMultiline("Text", &m_Text);
    ImGui::EndDisabled();

    ImGui::Checkbox("Scrollable", &m_IsScrollable);

    ImGui::BeginDisabled(m_Mode != Mode::SingleLine);
    int alignment = static_cast<int>(m_Alignment);
    if (ImGui::Combo("Alignment", &alignment, "Left\0Right\0Center\0"))
    {
        m_Alignment = static_cast<Alignment>(alignment);
    }
    ImGui::EndDisabled();
}

void Text::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "margin", m_Margin, 0);
    TryDeserialize(data, "is_scrollable", m_IsScrollable, false);
    TryDeserialize<Mode>(data, "mode", m_Mode, Mode::SingleLine);
    TryDeserialize<Alignment>(data, "alignment", m_Alignment, Alignment::Left);
}

nlohmann::json Text::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();

    if (!HasFlag(Flags::Bound))
    {
        data["text"] = m_Text;
    }
    data["margin"] = m_Margin;
    data["is_scrollable"] = m_IsScrollable;
    data["mode"] = magic_enum::enum_name(m_Mode);
    data["alignment"] = magic_enum::enum_name(m_Alignment);
    return data;
}

void Text::SetText(const std::string& text)
{
    m_Text = text;
}

} // namespace WingsOfSteel::TheBrightestStar::UI