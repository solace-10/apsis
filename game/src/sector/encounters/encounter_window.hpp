#pragma once

#include <array>

#include <core/smart_ptr.hpp>

#include "ui/button.hpp"
#include "ui/dice.hpp"
#include "ui/heading.hpp"
#include "ui/image.hpp"
#include "ui/text.hpp"
#include "ui/window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Encounter);
DECLARE_SMART_PTR(EncounterStageNode);

DECLARE_SMART_PTR(EncounterWindow);
class EncounterWindow : public UI::Window
{
public:
    EncounterWindow() {}
    ~EncounterWindow() override {}

    void OnInitializationCompleted() override;

    void AppendText(const std::string& text);
    void ClearText();

    void SetCurrentStage(Encounter* pEncounter, EncounterStageNodeSharedPtr pStage);

private:
    EncounterStageNodeWeakPtr m_pCurrentStage;
    std::array<UI::ButtonSharedPtr, 3> m_pEncounterButtons;
    //UI::ImageSharedPtr m_pEncounterImage;
    UI::TextSharedPtr m_pEncounterText;

    struct ShipElements
    {
        UI::HeadingSharedPtr pShipName;
        std::array<UI::DiceSharedPtr, 2> ShipDice; 
    };
    std::array<ShipElements, 3> m_ShipElements;
};

} // namespace WingsOfSteel::TheBrightestStar