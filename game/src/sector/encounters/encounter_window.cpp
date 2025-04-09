#include <string>

#include <imgui.h>

#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "sector/encounters/encounter_window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void EncounterWindow::OnInitializationCompleted()
{
    m_pEncounterText = FindElement<UI::Text>("/encounter_window/stack/encounter_text_stack/encounter_text");
    m_pEncounterButtons[0] = FindElement<UI::EncounterButton>("/encounter_window/stack/encounter_text_stack/button_option_1");
    m_pEncounterButtons[1] = FindElement<UI::EncounterButton>("/encounter_window/stack/encounter_text_stack/button_option_2");
    m_pEncounterButtons[2] = FindElement<UI::EncounterButton>("/encounter_window/stack/encounter_text_stack/button_option_3");
}

void EncounterWindow::AppendText(const std::string& text)
{
    if (m_pEncounterText)
    {
        std::string currentText = m_pEncounterText->GetText();
        if (currentText.empty())
        {
            m_pEncounterText->SetText(text);
        }
        else
        {
            currentText += "---\n\n" + text;
            m_pEncounterText->SetText(currentText);
        }
    }
}

void EncounterWindow::ClearText()
{
    if (m_pEncounterText)
    {
        m_pEncounterText->SetText("");
    }
}

void EncounterWindow::SetCurrentStage(EncounterStageNodeSharedPtr pStage)
{
    m_pCurrentStage = pStage;
    for (int i = 0; i < 3; ++i)
    {
        if (m_pEncounterButtons[i])
        {
            m_pEncounterButtons[i]->SetOnClickedEvent(
                [this, i]()
                {
                    EncounterStageNodeSharedPtr pStage = m_pCurrentStage.lock();
                    if (pStage)
                    {
                        // This logic will likely get more complicated if we have fewer than 3 options.
                        const EncounterStageNode::Option option = static_cast<EncounterStageNode::Option>(i);
                        pStage->OnOptionSelected(option);
                    }
                }
            );
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar