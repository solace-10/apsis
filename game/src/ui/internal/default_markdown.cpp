#include "ui/internal/default_markdown.hpp"

namespace WingsOfSteel::UI::Internal
{

static bool sMarkdownConfigInitialized = false;
static ImGui::MarkdownConfig sDefaultMarkdownConfig;

const ImGui::MarkdownConfig& DefaultMarkdown::Get()
{
    if (!sMarkdownConfigInitialized)
    {
        sMarkdownConfigInitialized = true;
    }

    sDefaultMarkdownConfig.linkCallback =         LinkCallback;
    sDefaultMarkdownConfig.tooltipCallback =      nullptr; //ImGui::defaultMarkdownTooltipCallback;
    sDefaultMarkdownConfig.imageCallback =        nullptr;
    sDefaultMarkdownConfig.linkIcon =             ICON_FA_LINK;
    //sDefaultMarkdownConfig.headingFormats[0] =    { H1, true };
    //sDefaultMarkdownConfig.headingFormats[1] =    { H2, true };
    //sDefaultMarkdownConfig.headingFormats[2] =    { H3, false };
    sDefaultMarkdownConfig.userData =             nullptr;
    sDefaultMarkdownConfig.formatCallback =       FormatCallback;

    return sDefaultMarkdownConfig;
}

void DefaultMarkdown::FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start)
{
    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo, start);        
       
    switch (markdownFormatInfo.type)
    {
        case ImGui::MarkdownFormatType::HEADING:
        {
            if (markdownFormatInfo.level == 2)
            {
                if(start)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }
            break;
        }
        case ImGui::MarkdownFormatType::EMPHASIS:
        {
            if (start)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.02f, 0.98f, 0.75f, 1.0f));
            }
            else
            {
                ImGui::PopStyleColor();
            }
            
            break;
        }
        case ImGui::MarkdownFormatType::LINK:
        {
            static const ImVec4 sDefaultColor(0.95f, 0.68f, 0.60f, 1.00f);
            static const ImVec4 sHoveredColor(1.00f, 0.88f, 0.85f, 1.00f);

            if (start)
            {
                // TODO:
                // This doesn't actually work for coloring the entire word as the `itemHovered` flag isn't set until
                // after the link text is written.
                // The markdown renderer needs to be modified to support this, in `TextRegion::RenderLinkText`.
                ImGui::PushStyleColor(ImGuiCol_Text, markdownFormatInfo.itemHovered ? sHoveredColor : sDefaultColor);
            }
            else
            {
                ImGui::PopStyleColor();
                ImGui::UnderLine(markdownFormatInfo.itemHovered ? sHoveredColor : sDefaultColor);
            }
        }
        default:
        {
            break;
        }
    }
}

void DefaultMarkdown::LinkCallback(ImGui::MarkdownLinkCallbackData data)
{
    std::string url(data.link, data.linkLength);
    if (!data.isImage)
    {
        
    }
}

} // namespace WingsOfSteel::UI::Internal