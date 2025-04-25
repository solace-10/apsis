#include <array>

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

#include <imgui/imgui.hpp>

#include "ui/dice.hpp"
#include "ui/theme.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Dice::Dice()
{
    AddFlag(Flags::AutoSize);
}

ElementType Dice::GetType() const
{
    return ElementType::Dice;
}

const std::string& Dice::GetIcon() const
{
    static const std::string icon(ICON_FA_DICE_FIVE);
    return icon;
}

void Dice::Render()
{
    const ImVec2 contentSize = GetCellSize();
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + contentSize;

    ImGui::SetCursorScreenPos(cp0);
    if (contentSize.x > 0.0f && contentSize.y > 0.0f && ImGui::InvisibleButton(GetName().c_str(), contentSize) && m_OnClickedEvent && !HasFlag(Flags::Selected))
    {
        m_OnClickedEvent();
    }
    ImGui::SetCursorScreenPos(cp0); // This needs to be set again so the cursor position is correct as the InvisibleButton modifies it.
    
    const bool isHighlighted = (ImGui::IsItemHovered() && m_OnClickedEvent) || HasFlag(Flags::Selected);
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    int notchSize = 12;
    std::array<ImVec2, 6> background =
    {
        cp0 + ImVec2(notchSize, 0),
        cp0 + ImVec2(contentSize.x, 0),
        cp1 - ImVec2(0, notchSize),
        cp1 - ImVec2(notchSize, 0),
        cp0 + ImVec2(0, contentSize.y),
        cp0 + ImVec2(0, notchSize)
    };
    pDrawList->AddConvexPolyFilled(
        background.data(),
        background.size(),
        IM_COL32(0, 0, 0, 120)
    );

    if (isHighlighted && m_Dice.has_value())
    {
        pDrawList->AddPolyline(
            background.data(),
            background.size(),
            Theme::AccentColor,
            ImDrawFlags_Closed, 1.5f
        );
    }

    m_AnimationTimer += ImGui::GetIO().DeltaTime;

    const ImVec2 center = cp0 + contentSize / 2;
    const float scale = contentSize.x / 96.0f;

    if (m_Dice.has_value())
    {
        const int dieValue = m_Dice.value().GetValue();
        if (dieValue == 1)
        {
            pDrawList->AddCircleFilled(center, 10.0f * scale, Theme::DieNegativeColor);
            pDrawList->AddEllipse(center, ImVec2(18.0f * scale + glm::cos(m_AnimationTimer) * 18.0f * scale, 36.0f * scale), Theme::DieNegativeColor, 0.0f, 0, 1.5f);
        }
        else if (dieValue == 2)
        {
            const int radius = static_cast<int>(28.0f * scale);
            pDrawList->AddCircle(center, radius, Theme::DieNegativeColor, 0, 1.5f);

            const ImVec2 orbit(glm::cos(m_AnimationTimer) * radius, glm::sin(m_AnimationTimer) * radius);
            pDrawList->AddCircleFilled(center + orbit, 8.0f * scale, Theme::DieNegativeColor);
            pDrawList->AddCircleFilled(center - orbit, 8.0f * scale, Theme::DieNegativeColor);
        }
        else if (dieValue == 3)
        {
            const int radius = static_cast<int>(28.0f * scale);
            pDrawList->AddCircle(center, radius, Theme::DieNeutralColor, 0, 1.5f);

            const float angle1 = m_AnimationTimer;
            const float angle2 = m_AnimationTimer + 2.0f * glm::pi<float>() / 3.0f;
            const float angle3 = m_AnimationTimer + 4.0f * glm::pi<float>() / 3.0f;

            const std::array<ImVec2, 3> orbits =
            {
                center + ImVec2(glm::cos(angle1) * radius, glm::sin(angle1) * radius),
                center + ImVec2(glm::cos(angle2) * radius, glm::sin(angle2) * radius),
                center + ImVec2(glm::cos(angle3) * radius, glm::sin(angle3) * radius)
            };

            pDrawList->AddCircleFilled(orbits[0], 8.0f * scale, Theme::DieNeutralColor);
            pDrawList->AddCircleFilled(orbits[1], 8.0f * scale, Theme::DieNeutralColor);
            pDrawList->AddCircleFilled(orbits[2], 8.0f * scale, Theme::DieNeutralColor);

            pDrawList->AddPolyline(orbits.data(), orbits.size(), Theme::DieNeutralColor, ImDrawFlags_Closed, 1.5f);
        }
        else if (dieValue == 4)
        {
            const int outerRadius = static_cast<float>(36.0f * scale);
            pDrawList->AddCircle(center, outerRadius, Theme::DieNeutralColor, 0, 1.5f);

            const int innerRadius = static_cast<float>(20.0f * scale);
            pDrawList->AddCircleFilled(center, 8.0f * scale, Theme::DieNeutralColor);

            const std::array<float, 3> angles =
            {
                m_AnimationTimer,
                m_AnimationTimer + 2.0f * glm::pi<float>() / 3.0f,
                m_AnimationTimer + 4.0f * glm::pi<float>() / 3.0f
            };

            const std::array<ImVec2, 3> orbits =
            {
                ImVec2(glm::cos(angles[0]) * innerRadius, glm::sin(angles[0]) * innerRadius),
                ImVec2(glm::cos(angles[1]) * innerRadius, glm::sin(angles[1]) * innerRadius),
                ImVec2(glm::cos(angles[2]) * innerRadius, glm::sin(angles[2]) * innerRadius)
            };

            for (const ImVec2& orbit : orbits)
            {
                pDrawList->AddCircleFilled(center + orbit, 8.0f * scale, Theme::DieNeutralColor);
                pDrawList->AddLine(center, center + orbit, Theme::DieNeutralColor, 1.5f);
            }
        }
        else if (dieValue == 5)
        {
            const int offset = static_cast<float>(28.0f * scale);
            std::array<ImVec2, 5> points =
            { 
                center - ImVec2(offset, offset),
                center - ImVec2(offset, -offset),
                center + ImVec2(0, glm::sin(m_AnimationTimer) * offset * 0.75f),
                center - ImVec2(-offset, -offset),
                center - ImVec2(-offset, offset)
            };

            for (const ImVec2& point : points)
            {
                pDrawList->AddCircleFilled(point, 8.0f * scale, Theme::DiePositiveColor);
            }

            pDrawList->AddPolyline(points.data(), points.size(), Theme::DiePositiveColor, ImDrawFlags_None, 1.5f);
        }
        else if (dieValue == 6)
        {
            pDrawList->AddCircleFilled(center, 10.0f * scale, Theme::DiePositiveColor);
            for (int i = 0; i < 5; i++)
            {
                const float halfRadius = (18.0f - static_cast<float>(i) * 2.0f) * scale;
                const float animationTimer = m_AnimationTimer * (1.0f - static_cast<float>(i) / 10.0f);
                pDrawList->AddEllipse(center, ImVec2(halfRadius + glm::cos(animationTimer) * halfRadius, halfRadius * 2.0f), Theme::DiePositiveColor, 0.0f, 0, 1.5f);
            }
        }
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI