#include <imgui/imgui.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "ui/encounter_button.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

EncounterButton::EncounterButton()
{
    AddFlag(Flags::AutoSize);
}

ElementType EncounterButton::GetType() const
{
    return ElementType::EncounterButton;
}

const std::string& EncounterButton::GetIcon() const
{
    static const std::string icon(ICON_FA_CODE_BRANCH);
    return icon;
}

void EncounterButton::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + glm::vec2(m_Margin);
    const ImVec2 cp1 = cp0 + GetCellSize() - glm::vec2(m_Margin * 2);

    ImGui::SetCursorScreenPos(cp0);

    const ImVec2 screenSize = GetCellSize() - glm::vec2(m_Margin * 2);
    const ImVec2 iconSize(screenSize.y, screenSize.y);
    if (ImGui::InvisibleButton(GetName().c_str(), screenSize) && m_OnClickedEvent)
    {
        m_OnClickedEvent();
    }

    ImGui::SetCursorScreenPos(cp0); // This needs to be set again so the cursor position is correct as the InvisibleButton modifies it.
    
    const bool hovered = ImGui::IsItemHovered();

    ImU32 accentColor = IM_COL32(5, 250, 191, 255);

    std::array<ImVec2, 5> buttonBorder;
    buttonBorder[ 0 ] = ImVec2(cp0.x + 16.0f, cp0.y);
    buttonBorder[ 1 ] = ImVec2(cp1.x, cp0.y);
    buttonBorder[ 2 ] = ImVec2(cp1.x, cp1.y - 8);
    buttonBorder[ 3 ] = ImVec2(cp1.x - 8, cp1.y - 1);
    buttonBorder[ 4 ] = ImVec2(cp0.x + 16.0f, cp1.y - 1);

    for (auto& v : buttonBorder)
    {
        v += ImVec2(0.5f, 0.5f);
    }

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    if (hovered)
    {
        pDrawList->AddConvexPolyFilled(buttonBorder.data(), static_cast<int>(buttonBorder.size()), IM_COL32(5, 250, 191, 80));
    }

    if (m_pIconTexture)
    {   
        const ImVec2 iconPosition = cp0;
        pDrawList->AddImageRounded( 
            reinterpret_cast<ImTextureID>(m_pIconTexture->GetTextureView().Get()),
            iconPosition,
            iconPosition + iconSize,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            accentColor,
            4.0f
        );
    }

    pDrawList->AddPolyline(buttonBorder.data(), static_cast<int>(buttonBorder.size()), accentColor, 0 , 1.0f);

    if (!m_Text.empty())
    {
        ImGui::SetCursorScreenPos(cp0 + glm::vec2(48.0f, 7.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, hovered ? accentColor : IM_COL32(5, 250, 191, 180));
        ImGui::Text("%s", m_Text.c_str());
        ImGui::PopStyleColor();
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void EncounterButton::RenderProperties()
{
    StackableElement::RenderProperties();
    ImGui::InputInt("Margin", &m_Margin);
    ImGui::InputTextMultiline("Text", &m_Text);

    std::string& iconSource = m_IconSource;
    if (ImGui::InputText("Icon", &iconSource))
    {
        SetIconSource(iconSource);
    }
}

void EncounterButton::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize(data, "margin", m_Margin, 0);

    std::string iconSource;
    if (TryDeserialize(data, "icon", iconSource, ""))
    {
        SetIconSource(iconSource);
    }
}

nlohmann::json EncounterButton::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["text"] = m_Text;
    data["margin"] = m_Margin;
    data["icon"] = m_IconSource;
    return data;
}

void EncounterButton::SetText(const std::string& text)
{
    m_Text = text;
}

void EncounterButton::SetIconSource(const std::string& source)
{
    m_IconSource = source;
    m_pIconTexture.reset();

    using namespace Pandora;
    if (GetVFS()->Exists(m_IconSource))
    {
        GetResourceSystem()->RequestResource(m_IconSource, [this](ResourceSharedPtr pResource)
        {
            if (pResource)
            {
                m_pIconTexture = std::static_pointer_cast<ResourceTexture2D>(pResource);
            }
        });
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI