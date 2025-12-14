#pragma once

#include "core/signal.hpp"

#include "ui/text.hpp"
#include "ui/window.hpp"

namespace WingsOfSteel
{

enum class EncounterResult;

DECLARE_SMART_PTR(VictoryWindow);
class VictoryWindow : public UI::Window
{
public:
    VictoryWindow();
    ~VictoryWindow() override;

    void OnInitializationCompleted() override;

private:
    void OnEncounterResolved(EncounterResult encounterResult);

    SignalId m_EncounterResolvedSignalId{ InvalidSignalId };

    UI::TextSharedPtr m_pDeploymentDurationText;
    UI::TextSharedPtr m_pKillsText;
};

} // namespace WingsOfSteel
