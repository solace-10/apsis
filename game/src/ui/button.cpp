#include <glm/glm.hpp>
#include <magic_enum.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_system.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "ui/button.hpp"
#include "ui/theme.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Button::Button()
{
    AddFlag(Flags::AutoSize);
}

ElementType Button::GetType() const
{
    return ElementType::Button;
}

const std::string& Button::GetIcon() const
{
    static const std::string icon(ICON_FA_CODE_BRANCH);
    return icon;
}

void Button::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + glm::vec2(m_Margin);
    const ImVec2 cp1 = cp0 + GetCellSize() - glm::vec2(m_Margin * 2);

    ImGui::SetCursorScreenPos(cp0);

    const ImVec2 screenSize = GetCellSize() - glm::vec2(m_Margin * 2);
    const ImVec2 iconSize(screenSize.y, screenSize.y);
    if (screenSize.x > 0.0f && screenSize.y > 0.0f && ImGui::InvisibleButton(GetName().c_str(), screenSize) && m_OnClickedEvent)
    {
        m_OnClickedEvent();
    }

    ImGui::SetCursorScreenPos(cp0); // This needs to be set again so the cursor position is correct as the InvisibleButton modifies it.
    
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();

    if (m_Mode == Mode::Standard)
    {
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

        if (hovered)
        {
            pDrawList->AddConvexPolyFilled(buttonBorder.data(), static_cast<int>(buttonBorder.size()), Theme::ButtonHovered);
        }

        pDrawList->AddPolyline(buttonBorder.data(), static_cast<int>(buttonBorder.size()), Theme::AccentColor, 0 , 1.0f);
    }
    else
    {
        if (hovered)
        {
            pDrawList->AddRectFilled(cp0 + ImVec2(16.0f, 0.0f), cp1, Theme::ButtonHovered);
        }

        //m_BackgroundAnimation += ImGui::GetIO().DeltaTime;

        ImGui::PushClipRect(cp0, cp1, false);
        const int stepWidth = 32;
        const int numSteps = (static_cast<int>(screenSize.x) / stepWidth) + 1;
        const ImColor c1 = ImColor(0.02f, 0.98f, 0.75f, 0.2f);
        const ImColor c2 = ImColor(0.02f, 0.98f, 0.75f, 0.1f);

        m_BackgroundOffset += ImGui::GetIO().DeltaTime * stepWidth;
        if (m_BackgroundOffset > static_cast<float>(stepWidth * 2))
        {
            m_BackgroundOffset = 0.0f;
        }

        for (int i = -1; i < numSteps; i++)
        {
            ImVec2 v0(m_BackgroundOffset + cp0.x + i * stepWidth, cp0.y);
            ImVec2 v1(v0.x + stepWidth, cp0.y);
            ImVec2 v2(v0.x, cp0.y + screenSize.y);
            ImVec2 v3(v0.x - stepWidth, cp0.y + screenSize.y);
            pDrawList->AddQuadFilled(v0, v1, v2, v3, i % 2 == 0 ? c1 : c2);
        }
        ImGui::PopClipRect();

        pDrawList->AddRect(cp0 + ImVec2(16.0f, 0.0f), cp1, Theme::AccentColor);
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
            Theme::AccentColor,
            4.0f
        );
    }

    if (!m_Text.empty())
    {
        ImGui::SetCursorScreenPos(cp0 + glm::vec2(48.0f, 7.0f));
        const ImColor textColor = hovered ? Theme::ButtonTextHovered : Theme::ButtonText;
        ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(textColor));

        if (m_Mode == Mode::Standard)
        {
            ImGui::TextUnformatted(m_Text.c_str());
        }
        else
        {
            ImGui::PushFont(Pandora::GetImGuiSystem()->GetFont(Pandora::Font::EXO2_SEMIBOLD_22));
            ImGui::TextUnformatted(m_Text.c_str());
            ImGui::PopFont();
        }
        ImGui::PopStyleColor();
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImGui::GetWindowDrawList()->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
    }
}

void Button::RenderProperties()
{
    StackableElement::RenderProperties();

    int mode = static_cast<int>(m_Mode);
    if (ImGui::Combo("Mode", &mode, "Standard\0Important\0"))
    {
        m_Mode = static_cast<Mode>(mode);
    }

    ImGui::InputInt("Margin", &m_Margin);
    ImGui::InputTextMultiline("Text", &m_Text);

    std::string& iconSource = m_IconSource;
    if (ImGui::InputText("Icon", &iconSource))
    {
        SetIconSource(iconSource);
    }
}

void Button::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string text;
    TryDeserialize(data, "text", text, "<placeholder>");
    SetText(text);

    TryDeserialize<Mode>(data, "mode", m_Mode, Mode::Standard);
    TryDeserialize(data, "margin", m_Margin, 0);

    std::string iconSource;
    if (TryDeserialize(data, "icon", iconSource, ""))
    {
        SetIconSource(iconSource);
    }
}

nlohmann::json Button::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["mode"] = magic_enum::enum_name(m_Mode);
    data["text"] = m_Text;
    data["margin"] = m_Margin;
    data["icon"] = m_IconSource;
    return data;
}

void Button::SetText(const std::string& text)
{
    m_Text = text;
}

void Button::SetIconSource(const std::string& source)
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