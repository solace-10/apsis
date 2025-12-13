#pragma once

#include <array>
#include <vector>

#include <core/signal.hpp>
#include <core/smart_ptr.hpp>
#include <imgui/idebugui.hpp>
#include <scene/scene.hpp>

#include "sector/deck/deck.hpp"
#include "sector/wing.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(DefeatWindow);
DECLARE_SMART_PTR(Sector);
DECLARE_SMART_PTR(VictoryWindow);

enum class EncounterResult
{
    Undecided,
    Victory,
    Defeat  
};


DECLARE_SMART_PTR(Encounter);
class Encounter : public IDebugUI
{
public:
    Encounter() = default;
    ~Encounter() = default;

    void Initialize(SectorSharedPtr pSector);
    void Update(float delta);

    EntitySharedPtr GetCarrier() { return m_pCarrier.lock(); }
    EncounterResult GetEncounterResult() const { return m_EncounterResult; }

    Signal<EncounterResult>& GetEncounterResolvedSignal() { return m_EncounterResolvedSignal; }

    void DrawDebugUI() override;

private:
    void SpawnCarrier();
    void EvaluateEncounterStarted();
    void EvaluateEscalation();
    void EscalateTier();
    void EvaluateEncounterResult();

    struct EncounterTier
    {
        DeckUniquePtr pDeck;
        float timeBetweenActions{ 30.0f };  
    };

    EncounterResult m_EncounterResult{ EncounterResult::Undecided };
    EntityWeakPtr m_pCarrier;
    std::vector<WingUniquePtr> m_Wings;
    SectorWeakPtr m_pSector;    
    std::array<EncounterTier, 3> m_EncounterTiers;
    int m_CurrentTier{0};
    float m_TimeToNextAction{ 0.0f };
    bool m_HasEncounterStarted{ false };

    Signal<EncounterResult> m_EncounterResolvedSignal;

    VictoryWindowSharedPtr m_pVictoryWindow;
    DefeatWindowSharedPtr m_pDefeatWindow;
};

} // namespace WingsOfSteel
