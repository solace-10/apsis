#pragma once

#include <array>

#include <core/smart_ptr.hpp>

#include "ui/encounter_button.hpp"
#include "ui/image.hpp"
#include "ui/text.hpp"
#include "ui/window.hpp"

namespace WingsOfSteel::TheBrightestStar
{

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

    void SetCurrentStage(EncounterStageNodeSharedPtr pStage);

private:
    EncounterStageNodeWeakPtr m_pCurrentStage;
    std::array<UI::EncounterButtonSharedPtr, 3> m_pEncounterButtons;
    //UI::ImageSharedPtr m_pEncounterImage;
    UI::TextSharedPtr m_pEncounterText;
};

} // namespace WingsOfSteel::TheBrightestStar