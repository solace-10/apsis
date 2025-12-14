#include <core/log.hpp>

#include "game.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "sector/victory_window.hpp"

namespace WingsOfSteel
{

VictoryWindow::VictoryWindow()
{
}

VictoryWindow::~VictoryWindow()
{
    if (m_EncounterResolvedSignalId != InvalidSignalId)
    {
        Encounter* pEncounter = Game::Get()->GetSector()->GetEncounter();
        if (pEncounter)
        {
            pEncounter->GetEncounterResolvedSignal().Disconnect(m_EncounterResolvedSignalId);
        }
    }
}

void VictoryWindow::OnInitializationCompleted()
{
    m_pDeploymentDurationText = FindElement<UI::Text>("/victory_window/stack_h/stack_v/stats_stack_h/stats_stack_v/stat_2_stack/stat_2_value");
    m_pKillsText = FindElement<UI::Text>("/victory_window/stack_h/stack_v/stats_stack_h/stats_stack_v/stat_3_stack/stat_3_value");

    Encounter* pEncounter = Game::Get()->GetSector()->GetEncounter();
    if (!pEncounter)
    {
        Log::Error() << "Can't find encounter?";
        return;
    }

    m_EncounterResolvedSignalId = pEncounter->GetEncounterResolvedSignal().ConnectMember(this, &VictoryWindow::OnEncounterResolved);
}

void VictoryWindow::OnEncounterResolved(EncounterResult encounterResult)
{
    if (encounterResult != EncounterResult::Victory)
    {
        return;
    }

    const Encounter* pEncounter = Game::Get()->GetSector()->GetEncounter();
    if (pEncounter)
    {
        const EncounterStats& stats = pEncounter->GetEncounterStats();
        const auto duration = std::chrono::duration_cast<std::chrono::seconds>(stats.finishedAt - stats.startedAt);
        const int totalSeconds = static_cast<int>(duration.count());
        const int minutes = totalSeconds / 60;
        const int seconds = totalSeconds % 60;
        m_pDeploymentDurationText->SetText(std::to_string(minutes) + "m" + std::to_string(seconds) + "s");
        m_pKillsText->SetText(std::to_string(stats.kills));
    }
}

} // namespace WingsOfSteel
