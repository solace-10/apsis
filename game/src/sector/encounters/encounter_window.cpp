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

    m_DieDetails.pDie = FindElement<UI::Dice>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/chosen_die");
    m_DieDetails.pHeading = FindElement<UI::Heading>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/die_heading");
    m_DieDetails.pBreakdownPositive = FindElement<UI::Text>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/die_breakdown/die_positive");
    m_DieDetails.pBreakdownNeutral = FindElement<UI::Text>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/die_breakdown/die_neutral");
    m_DieDetails.pBreakdownNegative = FindElement<UI::Text>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/die_breakdown/die_negative");
    m_DieDetails.pModifiers = FindElement<UI::Text>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/die_modifiers");

    m_pConfirmButton = FindElement<UI::Button>("/encounter_window/stack/encounter_image_stack/stack/stack_h/stack_v/details_stack_h/die_details/button_confirm");
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
                    DiceCategory diceCategory = pDiceNode->Value;
                    static const std::array<std::string, magic_enum::enum_count<DiceCategory>()> categoryIcons =
                    {
                        "/ui/icons/encounter_button_electronics.png",
                        "/ui/icons/encounter_button_engineering.png",
                        "/ui/icons/encounter_button_jump.png",
                        "/ui/icons/encounter_button_jump.png",
                        "/ui/icons/encounter_button_jump.png"
                    };
                    pEncounterButton->SetIconSource(categoryIcons[static_cast<size_t>(diceCategory)]);

                    pEncounterButton->SetOnClickedEvent(
                        [this, pEncounterButton, i, diceCategory]()
                        {
                            m_pEncounterButtonGroup->Select(pEncounterButton);
                            SetCurrentStageOption(diceCategory, i);
                        }
                    );
                }
                else
                {
                    static const std::string jumpIcon("/ui/icons/encounter_button_jump.png");
                    pEncounterButton->SetIconSource(jumpIcon);
                }
            }
            else
            {
                pEncounterButton->SetText("...");
                pEncounterButton->AddFlag(Element::Flags::Disabled);
            }
        }
    }
}

void EncounterWindow::SetCurrentStageOption(DiceCategory dieCategory, int optionIndex)
{
    m_SelectedStageOption = optionIndex;
    m_pSelectionPanel->RemoveFlag(Flags::Hidden);

    entt::registry& registry = Pandora::GetActiveScene()->GetRegistry();
    auto view = registry.view<const DiceComponent, const NameComponent>();

    m_SelectedDie = std::nullopt;
    size_t shipIndex = 0;
    for (auto entity : view)
    {
        const auto& nameComponent = view.get<NameComponent>(entity);
        m_ShipElements[shipIndex].pShipName->SetText(nameComponent.Value);

        const auto& diceComponent = view.get<DiceComponent>(entity);
        const auto& diceContainer = diceComponent.GetDice(dieCategory);
        for (size_t dieIndex = 0; dieIndex < diceContainer.size(); dieIndex++)
        {
            UI::DiceSharedPtr& pUIDie = m_ShipElements[shipIndex].ShipDice[dieIndex];
            std::optional<Dice> die = diceContainer[dieIndex];
            if (die.has_value())
            {
                pUIDie->SetDice(die.value());

                if (!m_SelectedDie.has_value())
                {
                    pUIDie->AddFlag(UI::Element::Flags::Selected);
                    SetCurrentSelectedDie(dieCategory, die.value(), optionIndex);
                }
                else
                {
                    pUIDie->RemoveFlag(UI::Element::Flags::Selected);
                }

                pUIDie->SetOnClickedEvent([this, dieCategory, die, optionIndex](){
                    if (die.has_value())
                    {
                        SetCurrentSelectedDie(dieCategory, die.value(), optionIndex);
                    }
                });
            }
            else
            {
                pUIDie->ClearDice();
                pUIDie->RemoveFlag(UI::Element::Flags::Selected);
                pUIDie->SetOnClickedEvent([](){});
            }
        }
        shipIndex++;
    }
}

void EncounterWindow::SetCurrentSelectedDie(DiceCategory dieCategory, const Dice& die, int optionIndex)
{
    m_SelectedDie = die;
    m_DieDetails.pDie->SetDice(die);

    static const std::array<std::string, 5> diceCategories = { "ELECTRONICS", "ENGINEERING", "NAVIGATION", "SCIENCE", "WARFARE" }; 
    static const std::array<std::string, 6> diceNumerals = { "I", "II", "III", "IV", "V", "VI" };
    std::stringstream heading;
    heading << diceCategories[static_cast<size_t>(dieCategory)] << " " << diceNumerals[static_cast<size_t>(die.GetValue() - 1)];
    m_DieDetails.pHeading->SetText(heading.str());

    std::stringstream positive;
    positive << "POS: " << static_cast<int>(die.GetPositiveChance() * 100.0f) << "%";
    m_DieDetails.pBreakdownPositive->SetText(positive.str());

    std::stringstream neutral;
    neutral << "NEU: " << static_cast<int>(die.GetNeutralChance() * 100.0f) << "%";
    m_DieDetails.pBreakdownNeutral->SetText(neutral.str());

    std::stringstream negative;
    negative << "NEG: " << static_cast<int>(die.GetNegativeChance() * 100.0f) << "%";
    m_DieDetails.pBreakdownNegative->SetText(negative.str());

    m_DieDetails.pModifiers->SetText("None");

    for (auto& shipElement : m_ShipElements)
    {
        for (auto& pUIDie : shipElement.ShipDice)
        {
            std::optional<Dice> uiDie = pUIDie->GetDice();
            if (uiDie.has_value() && uiDie.value().GetId() == die.GetId())
            {
                pUIDie->AddFlag(Flags::Selected);
            }
            else
            {
                pUIDie->RemoveFlag(Flags::Selected);
            }
        }
    }

    m_pConfirmButton->SetOnClickedEvent(
        [this, optionIndex, die]()
        {
            EncounterStageNodeSharedPtr pStage = m_pCurrentStage.lock();
            if (pStage)
            {
                const EncounterStageNode::Option option = static_cast<EncounterStageNode::Option>(optionIndex);
                pStage->OnOptionSelected(option, die);
            }
        }
    );
}

} // namespace WingsOfSteel::TheBrightestStar