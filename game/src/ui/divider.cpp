#include <imgui/imgui.hpp>

#include "ui/divider.hpp"
#include "ui/theme.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Divider::Divider()
{
    AddFlag(Flags::AutoSize);
}

ElementType Divider::GetType() const
{
    return ElementType::Divider;
}

const std::string& Divider::GetIcon() const
{
    static const std::string icon(ICON_FA_DIVIDE);
    return icon;
}

void Divider::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    const ImVec2 cp1 = cp0 + GetCellSize();

    const float halfPoint = cp0.y + (cp1.y - cp0.y) / 2.0f;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(cp0.x, halfPoint),
        ImVec2(cp1.x, halfPoint),
        Theme::AccentColor,
        1.5f
    );

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI