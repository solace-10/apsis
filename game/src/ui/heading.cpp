#include <array>

#include <magic_enum.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "ui/heading.hpp"
#include "ui/theme.hpp"

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

    if (m_Text.empty())
    {
        return;
    }

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    if (m_HeadingLevel == HeadingLevel::Heading1)
    {
        int w = 192;
        int h = 42;
        const int slant = 32;
        const int halfSlant = slant / 2;
        std::array<ImVec2, 4> verts =
        {
            cp0,
            ImVec2(cp0.x + w, cp0.y),
            ImVec2(cp0.x + w - slant, cp0.y + h),
            ImVec2(cp0.x, cp0.y + h)
        };
        pDrawList->AddConvexPolyFilled(verts.data(), verts.size(), IM_COL32(255, 255, 255, 30));

        int offset = cp0.x + w;
        w = 32;
        for (int i = 0; i < 4; i++)
        {
            std::array<ImVec2, 4> detailVerts =
            {
                ImVec2(offset + halfSlant, cp0.y),
                ImVec2(offset + w + halfSlant, cp0.y),
                ImVec2(offset + w - halfSlant, cp0.y + h),
                ImVec2(offset - halfSlant, cp0.y + h)
            };
            pDrawList->AddConvexPolyFilled(detailVerts.data(), detailVerts.size(), IM_COL32(255, 255, 255, 30 - i * 8));
            offset += 44;
        }

        if (!m_Text.empty())
        {
            ImGui::SetCursorScreenPos(cp0 + ImVec2(8, 4));
            ImGui::PushFont(Pandora::GetImGuiSystem()->GetFont(Pandora::Font::EXO2_SEMIBOLD_32));
            ImGui::TextUnformatted(m_Text.c_str());
            ImGui::PopFont();
        }
    }
    else if (m_HeadingLevel == HeadingLevel::Heading2)
    {
        if (!m_Text.empty())
        {
            ImGui::PushFont(Pandora::GetImGuiSystem()->GetFont(Pandora::Font::EXO2_SEMIBOLD_22));
            ImGui::TextUnformatted(m_Text.c_str());
            ImGui::PopFont();
        }

        const ImVec2 titleUnderlineStart(cp0 + ImVec2(0, 24));
        const ImVec2 titleUnderlineEnd(titleUnderlineStart + ImVec2(contentSize.x, 0));
        pDrawList->AddLine(titleUnderlineStart, titleUnderlineEnd, Theme::AccentColor);
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Heading::RenderProperties()
{
    StackableElement::RenderProperties();

    ImGui::BeginDisabled(m_IsDynamic);
    ImGui::InputTextMultiline("Text", &m_Text);
    ImGui::EndDisabled();

    ImGui::Checkbox("Dynamic", &m_IsDynamic);

    int level = static_cast<int>(m_HeadingLevel);
    if (ImGui::Combo("Heading level", &level, "Level 1\0Level 2\0"))
    {
        m_HeadingLevel = static_cast<HeadingLevel>(level);
    }
}

void Heading::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "is_dynamic", m_IsDynamic, false);
    TryDeserialize<HeadingLevel>(data, "level", m_HeadingLevel, HeadingLevel::Heading1);
}

nlohmann::json Heading::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();

    if (!m_IsDynamic)
    {
        data["text"] = m_Text;
    }
    data["is_dynamic"] = m_IsDynamic;
    data["level"] = magic_enum::enum_name(m_HeadingLevel);
    return data;
}

} // namespace WingsOfSteel::TheBrightestStar::UI