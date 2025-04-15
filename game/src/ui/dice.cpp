#include <imgui/imgui.hpp>

#include "ui/dice.hpp"

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
    static const std::string icon(ICON_FA_DICE);
    return icon;
}

void Dice::Render()
{
    const ImVec2 contentSize = GetCellSize();
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + contentSize;

    ImGui::SetCursorScreenPos(cp0);

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    //pDrawList->AddRectFilled(cp0, cp1, IM_COL32(0, 0, 0, 120));

    int notchSize = 12;
    ImVec2 background[6] =
    {
        cp0 + ImVec2(notchSize, 0),
        cp0 + ImVec2(contentSize.x, 0),
        cp1 - ImVec2(0, notchSize),
        cp1 - ImVec2(notchSize, 0),
        cp0 + ImVec2(0, contentSize.y),
        cp0 + ImVec2(0, notchSize)
    };
    pDrawList->AddConvexPolyFilled(
        background,
        6,
        IM_COL32(0, 0, 0, 120)
    );

    m_AnimationTimer += ImGui::GetIO().DeltaTime;

    ImVec2 center = cp0 + contentSize / 2;
    pDrawList->AddCircleFilled(center, 10.0f, IM_COL32(220, 0, 0, 200));
    pDrawList->AddEllipse(center, ImVec2(18.0f + cosf(m_AnimationTimer) * 18.0f, 36.0f), IM_COL32(220, 0, 0, 200), 0.0f, 0, 1.5f);

    // ImVec2 bracketLeft[4] = 
    // {
    //     cp0 + ImVec2(8, 0),
    //     cp0,
    //     cp0 + ImVec2(0, contentSize.y),
    //     cp0 + ImVec2(8, contentSize.y)
    // };
    // pDrawList->AddPolyline(bracketLeft, 4, IM_COL32(0, 255, 0, 255), ImDrawFlags_None, 1.5f);

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Dice::RenderProperties()
{
    StackableElement::RenderProperties();
}

void Dice::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);
}

nlohmann::json Dice::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    return data;
}

} // namespace WingsOfSteel::TheBrightestStar::UI