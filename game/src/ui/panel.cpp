#include <imgui/imgui.hpp>

#include "ui/panel.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Panel::Panel()
{
    AddFlag(Flags::AutoSize);
}

ElementType Panel::GetType() const
{
    return ElementType::Panel;
}

const std::string& Panel::GetIcon() const
{
    static const std::string icon(ICON_FA_SQUARE);
    return icon;
}

void Panel::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + GetCellSize();

    static const ImU32 accentColor = IM_COL32(5, 250, 191, 255);
    static const ImU32 backgroundStartColor = IM_COL32(46, 46, 46, 180);
    static const ImU32 backgroundEndColor = IM_COL32(20, 20, 20, 180);
    static const float notchSize = 16.0f;
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    std::array<ImVec2, 5> background;
    background[ 0 ] = ImVec2(cp0.x, cp0.y);
    background[ 1 ] = ImVec2(cp1.x, cp0.y);
    background[ 2 ] = ImVec2(cp1.x, cp1.y - notchSize);
    background[ 3 ] = ImVec2(cp1.x - notchSize, cp1.y);
    background[ 4 ] = ImVec2(cp0.x, cp1.y);

    std::array<ImU32, 5> backgroundColors;
    backgroundColors[ 0 ] = backgroundStartColor;
    backgroundColors[ 1 ] = backgroundStartColor;
    backgroundColors[ 2 ] = backgroundEndColor;
    backgroundColors[ 3 ] = backgroundEndColor;
    backgroundColors[ 4 ] = backgroundEndColor;
    pDrawList->AddConvexPolyFilledMultiColor(background.data(), backgroundColors.data(), static_cast<int>(background.size()));

    const float accentHeight = 4.0f;
    pDrawList->AddRectFilled(ImVec2(cp0.x, cp0.y), ImVec2(cp1.x, cp0.y + accentHeight), accentColor);

    static const int gridAlpha = 6;
    static const ImU32 gridColor = IM_COL32(255, 255, 255, gridAlpha);
    static const float gridStep = 48.0f;
    static const float gridOffset = -16.0f;
    static const float gridThickness = 1.5f;
    for (float x = cp0.x + gridOffset; x < cp1.x; x += gridStep)
    {
        pDrawList->AddLine(ImVec2(x, cp0.y), ImVec2(x, cp1.y), gridColor, gridThickness);
    }
    for (float y = cp0.y + gridOffset; y < cp1.y; y += gridStep)
    {
        pDrawList->AddLine(ImVec2(cp0.x, y), ImVec2(cp1.x, y), gridColor, gridThickness);
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI