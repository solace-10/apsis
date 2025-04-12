#pragma once

#include <imgui/imgui.hpp>

namespace WingsOfSteel::TheBrightestStar::UI::Internal
{

class DefaultMarkdown
{
public:
    static const ImGui::MarkdownConfig& Get();

private:
    static void FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start);
    static void LinkCallback(ImGui::MarkdownLinkCallbackData data);
};

} // namespace WingsOfSteel::TheBrightestStar::UI::Internal