#include <imgui.h>

#include "ui/image.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

void Image::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + GetPosition();
    const ImVec2 cp1 = cp0 + GetSize();

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    pDrawList->AddRectFilled(cp0, cp1, IM_COL32(255, 0, 0, 40));
    pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    pDrawList->AddText(cp0 + ImVec2(8.0f, 8.0f), IM_COL32(255, 0, 0, 255), "Image missing");
}

} // namespace WingsOfSteel::TheBrightestStar::UI