#include "ui/internal/default_markdown.hpp"

namespace WingsOfSteel::TheBrightestStar::UI::Internal
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
    sDefaultMarkdownConfig.tooltipCallback =      nullptr;
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

} // namespace WingsOfSteel::TheBrightestStar::UI::Internal