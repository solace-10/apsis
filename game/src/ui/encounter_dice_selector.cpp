#include <imgui/imgui.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "ui/encounter_dice_selector.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

EncounterDiceSelector::EncounterDiceSelector()
{
    AddFlag(Flags::AutoSize);
}

ElementType EncounterDiceSelector::GetType() const
{
    return ElementType::EncounterDiceSelector;
}

const std::string& EncounterDiceSelector::GetIcon() const
{
    static const std::string icon(ICON_FA_DICE);
    return icon;
}

void EncounterDiceSelector::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + GetCellSize();

    ImGui::SetCursorScreenPos(cp0);

    //if (ImGui::InvisibleButton(GetName().c_str(), screenSize) && m_OnClickedEvent)
    // {
    //     m_OnClickedEvent();
    // }

    ImGui::SetCursorScreenPos(cp0); // This needs to be set again so the cursor position is correct as the InvisibleButton modifies it.
   


    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    //pDrawList->AddRectFilled(cp0, cp1, IM_COL32(0, 255, 0, 80));

    ImVec2 center = cp0 + GetCellSize() / 2.0f;

    static float rectWidth = 800.0f;
    static float rectHeight = 500.0f;
    ImVec2 halfSize(rectWidth / 2.0f, rectHeight / 2.0f);
    static float yOffset = 0.0f;

    const ImVec2 windowTl = center + ImVec2(0.0f, yOffset) - halfSize;
    const ImVec2 windowBr = center + ImVec2(0.0f, yOffset) + halfSize;
    RenderBackground(windowTl, windowBr);

    auto drawDie = [pDrawList](ImVec2 tl)
    {
        pDrawList->AddRectFilled(tl, tl + ImVec2(96.0f, 96.0f), IM_COL32(0, 255, 255, 80));
    };

    static const ImU32 accentColor = IM_COL32(5, 250, 191, 255);
    static const std::string shipNames[3] = { "EVERFLAME", "SKYFORGER", "ARBELLATRIS" };
    ImVec2 tl = ImVec2(center + ImVec2(0.0f, yOffset) - halfSize);
    for (int shipIndex = 0; shipIndex < 3; shipIndex++)
    {
        pDrawList->AddText(tl + ImVec2(32.0f, 16.0f), IM_COL32(255, 255, 255, 255), shipNames[shipIndex].c_str());

        const ImVec2 titleUnderlineStart(tl.x + 32, tl.y + 40);
        const ImVec2 titleUnderlineEnd(titleUnderlineStart + ImVec2(224, 0));
        pDrawList->AddLine(titleUnderlineStart, titleUnderlineEnd, accentColor);
        
        for (int dieIndex = 0; dieIndex < 2; dieIndex++)
        {
            drawDie(tl + ImVec2(32.0f, 56.0f) + ImVec2(128.0f * dieIndex, 0.0f));
        }

        tl += ImVec2(256.0f, 0.0f);
    }

    const ImVec2 separatorStart(windowTl + ImVec2(32, 168));
    const ImVec2 separatorEnd(separatorStart + ImVec2(736, 0));
    pDrawList->AddLine(separatorStart, separatorEnd, accentColor);


    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void EncounterDiceSelector::RenderBackground(const glm::vec2& tl, const glm::vec2& br)
{
    const ImVec2 cp0 = tl;
    const ImVec2 cp1 = br;

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
}

void EncounterDiceSelector::RenderProperties()
{
    StackableElement::RenderProperties();
}

void EncounterDiceSelector::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);
}

nlohmann::json EncounterDiceSelector::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    return data;
}

} // namespace WingsOfSteel::TheBrightestStar::UI