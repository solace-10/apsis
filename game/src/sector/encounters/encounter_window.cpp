#include <cassert>
#include <string>

#include <imgui.h>

#include "components/dice_component.hpp"
#include "components/name_component.hpp"
#include "sector/encounters/encounter_blueprint_nodes.hpp"
#include "sector/encounters/encounter_window.hpp"
#include "sector/encounters/encounter.hpp"
#include "sector/sector.hpp"
#include "fleet.hpp"
#include "game.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void EncounterWindow::OnInitializationCompleted()
{
    m_pEncounterText = FindElement<UI::Text>("/encounter_window/stack/encounter_text_stack/encounter_text");
    m_pEncounterButtons[0] = FindElement<UI::Button>("/encounter_window/stack/encounter_text_stack/button_option_1");
    m_pEncounterButtons[1] = FindElement<UI::Button>("/encounter_window/stack/encounter_text_stack/button_option_2");
    m_pEncounterButtons[2] = FindElement<UI::Button>("/encounter_window/stack/encounter_text_stack/button_option_3");

    m_pEncounterButtonGroup = std::make_shared<UI::ButtonGroup>();
    for (auto& pButton : m_pEncounterButtons)
    {
        m_pEncounterButtonGroup->Add(pButton);
    }

    m_ShipElements[0].pShipName = FindElement<UI::Heading>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/titles/heading_ship_1");
    m_ShipElements[0].ShipDice[0] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_1");
    m_ShipElements[0].ShipDice[1] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_2");

    m_ShipElements[1].pShipName = FindElement<UI::Heading>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/titles/heading_ship_2");
    m_ShipElements[1].ShipDice[0] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_3");
    m_ShipElements[1].ShipDice[1] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_4");

    m_ShipElements[2].pShipName = FindElement<UI::Heading>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/titles/heading_ship_3");
    m_ShipElements[2].ShipDice[0] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_5");
    m_ShipElements[2].ShipDice[1] = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/dice/die_6");

    m_pSelectionPanel = FindElement<UI::Element>("/encounter_window/stack/encounter_image_stack/stack");
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
            currentText += "\n\n___\n\n" + text;
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

void EncounterWindow::SetCurrentStage(Encounter* pEncounter, EncounterStageNodeSharedPtr pStage)
{
    m_pCurrentStage = pStage;

    if (!pEncounter || !pStage)
    {
        return;
    }

    m_pEncounterButtonGroup->Select(nullptr);
    m_pSelectionPanel->AddFlag(Flags::Hidden);

    std::vector<EncounterOptionNode*> encounterOptionNodes = pStage->GetLinkedEncounterNodes(pEncounter);
    size_t numBindings = m_pEncounterButtons.size();
    assert(encounterOptionNodes.size() == numBindings);

    for (size_t i = 0; i < numBindings; ++i)
    {
        UI::Button* pEncounterButton = m_pEncounterButtons[i].get();
        if (pEncounterButton)
        {
            EncounterOptionNode* pEncounterOptionNode = encounterOptionNodes[i];
            if (pEncounterOptionNode)
            {
                pEncounterButton->RemoveFlag(Element::Flags::Disabled);
                StringNode* pStringNode = pEncounterOptionNode->GetLinkedStringNode(pEncounter);
                if (pStringNode)
                {
                    pEncounterButton->SetText(pStringNode->Value);
                }
                else
                {
                    pEncounterButton->SetText("<MISSING DESCRIPTION>");
                }

                DiceNode* pDiceNode = pEncounterOptionNode->GetLinkedDiceNode(pEncounter);
                if (pDiceNode)
                {
                    static const std::array<std::string, magic_enum::enum_count<DiceCategory>()> categoryIcons =
                    {
                        "/ui/icons/encounter_button_electronics.png",
                        "/ui/icons/encounter_button_engineering.png",
                        "/ui/icons/encounter_button_jump.png",
                        "/ui/icons/encounter_button_jump.png",
                        "/ui/icons/encounter_button_jump.png"
                    };
                    pEncounterButton->SetIconSource(categoryIcons[static_cast<size_t>(pDiceNode->Value)]);
                }
                else
                {
                    static const std::string jumpIcon("/ui/icons/encounter_button_jump.png");
                    pEncounterButton->SetIconSource(jumpIcon);
                }

                pEncounterButton->SetOnClickedEvent(
                    [this, pEncounterButton, i]()
                    {
                        m_pEncounterButtonGroup->Select(pEncounterButton);
                        SetCurrentStageOption(i);

                        // EncounterStageNodeSharedPtr pStage = m_pCurrentStage.lock();
                        // if (pStage)
                        // {
                        //     // This logic will likely get more complicated if we have fewer than 3 options.
                        //     const EncounterStageNode::Option option = static_cast<EncounterStageNode::Option>(i);
                        //     pStage->OnOptionSelected(option);
                        // }
                    }
                );
            }
            else
            {
                pEncounterButton->SetText("...");
                pEncounterButton->AddFlag(Element::Flags::Disabled);
            }
        }
    }
}

void EncounterWindow::SetCurrentStageOption(int optionIndex)
{
    m_SelectedStageOption = optionIndex;
    m_pSelectionPanel->RemoveFlag(Flags::Hidden);

    entt::registry& registry = Pandora::GetActiveScene()->GetRegistry();
    auto view = registry.view<const DiceComponent, const NameComponent>();

    size_t shipIndex = 0;
    for (auto entity : view)
    {
        const auto& nameComponent = view.get<NameComponent>(entity);
        m_ShipElements[shipIndex].pShipName->SetText(nameComponent.Value);

        const auto& diceComponent = view.get<DiceComponent>(entity);
        const auto& diceContainer = diceComponent.GetDice(DiceCategory::Electronics);
        for (size_t dieIndex = 0; dieIndex < diceContainer.size(); dieIndex++)
        {
            std::optional<Dice> die = diceContainer[dieIndex];
            if (die.has_value())
            {
                m_ShipElements[shipIndex].ShipDice[dieIndex]->SetDice(die.value());
            }
            else
            {
                m_ShipElements[shipIndex].ShipDice[dieIndex]->ClearDice();
            }
        }
        shipIndex++;
    }
}

} // namespace WingsOfSteel::TheBrightestStar