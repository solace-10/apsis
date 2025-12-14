#include <glm/gtc/matrix_transform.hpp>

#include <core/log.hpp>
#include <core/random.hpp>
#include <core/serialization.hpp>
#include <imgui/imgui.hpp>
#include <pandora.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>

#include "components/hull_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/deck/deck.hpp"
#include "sector/defeat_window.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "sector/victory_window.hpp"

namespace WingsOfSteel
{

Encounter::~Encounter()
{
    if (m_OnEntityKilled != InvalidSignalId)
    {
        Sector* pSector = Game::Get()->GetSector();
        if (pSector)
        {
            pSector->GetEntityKilledSignal().Disconnect(m_OnEntityKilled);
        }
    }
}

void Encounter::Initialize(SectorSharedPtr pSector)
{
    m_pSector = pSector;

    // List all encounter files in the difficulty2 directory
    const std::string encountersPath("/encounters/difficulty2");
    const std::vector<std::string> encounterFiles = GetVFS()->List(encountersPath);
    if (encounterFiles.empty())
    {
        Log::Error() << "No encounter files found in '" << encountersPath << "'.";
        return;
    }

    // Pick a random encounter file
    const size_t randomIndex = Random::Get<size_t>(0, encounterFiles.size() - 1);
    const std::string& encounterFile = encounterFiles[randomIndex];

    // Load the encounter data
    SectorWeakPtr pWeakSector = pSector;
    GetResourceSystem()->RequestResource(encounterFile, [this, pWeakSector](ResourceSharedPtr pResource) {
        SectorSharedPtr pSector = pWeakSector.lock();
        if (!pSector)
        {
            return;
        }

        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (pDataStore)
        {
            auto tiersArray = Json::DeserializeArray(pDataStore.get(), pDataStore->Data(), "tiers");
            if (!tiersArray.has_value())
            {
                Log::Error() << "Encounter file missing 'tiers' array.";
                return;
            }

            const size_t numTiers = std::min(tiersArray.value().size(), m_EncounterTiers.size());
            for (size_t tierIndex = 0; tierIndex < numTiers; tierIndex++)
            {
                const auto& tierObject = tiersArray.value()[tierIndex];

                m_EncounterTiers[tierIndex].timeBetweenActions = Json::DeserializeFloat(pDataStore.get(), tierObject, "time_between_actions");

                DeckUniquePtr pDeck = std::make_unique<Deck>();
                pDeck->Initialize(pDataStore.get(), tierObject);
                m_EncounterTiers[tierIndex].pDeck = std::move(pDeck);
            }

            SpawnCarrier();

            m_pVictoryWindow = std::make_shared<VictoryWindow>();
            m_pVictoryWindow->Initialize("/ui/prefabs/victory.json");
            m_pVictoryWindow->AddFlag(UI::Element::Flags::Hidden);

            GetEncounterResolvedSignal().Connect([this](EncounterResult encounterResult) {
                if (encounterResult == EncounterResult::Victory)
                {
                    m_pVictoryWindow->RemoveFlag(UI::Element::Flags::Hidden);
                }
            });

            m_OnEntityKilled = pSector->GetEntityKilledSignal().ConnectMember(this, &Encounter::OnEntityKilled);
        }
    });
}

void Encounter::SpawnCarrier()
{
    SceneWeakPtr pWeakScene = m_pSector;
    SectorWeakPtr pWeakSector = m_pSector;
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/raiders/carrier.json", glm::translate(glm::mat4(1.0f), glm::vec3(250.0f, 0.0f, 0.0f)), [pWeakSector](EntitySharedPtr pEntity) {
        SectorSharedPtr pSector = pWeakSector.lock();
        if (pSector)
        {
            Encounter* pEncounter = pSector->GetEncounter();
            pEncounter->m_pCarrier = pEntity;
        }
    });
}

void Encounter::Update(float delta)
{
    if (!m_HasEncounterStarted)
    {
        EvaluateEncounterStarted();
        return;
    }

    if (!m_pCarrier.expired())
    {

        m_TimeToNextAction -= delta;
        if (m_TimeToNextAction < 0.0)
        {
            Deck* pCurrentDeck = m_EncounterTiers[m_CurrentTier].pDeck.get();
            if (pCurrentDeck->PlayNextCard())
            {
                m_TimeToNextAction = m_EncounterTiers[m_CurrentTier].timeBetweenActions;
            }
            else
            {
                EscalateTier();
            }
        }
    }

    EvaluateEscalation();
    EvaluateEncounterResult();

    if (m_pVictoryWindow)
    {
        m_pVictoryWindow->Render();
    }

    if (m_pDefeatWindow)
    {
        m_pDefeatWindow->Render();
    }

    DrawDebugUI();
}

void Encounter::EvaluateEncounterStarted()
{
    if (m_pCarrier.expired())
    {
        return;
    }

    SectorSharedPtr pSector = m_pSector.lock();
    if (!pSector)
    {
        return;
    }

    if (!pSector->GetPlayerMech() || !pSector->GetPlayerCarrier())
    {
        return;
    }

    m_EncounterStats.startedAt = std::chrono::steady_clock::now();
    m_HasEncounterStarted = true;
}

void Encounter::EvaluateEscalation()
{
    if (GetEncounterResult() != EncounterResult::Undecided)
    {
        return;
    }

    EntitySharedPtr pCarrier = m_pCarrier.lock();
    if (!pCarrier || !pCarrier->HasComponent<HullComponent>())
    {
        return;
    }

    // Don't trigger escalation logic at the highest tier, as that will just cause the deck to be reshuffled.
    if (m_CurrentTier == 2)
    {
        return;
    }

    const HullComponent& hullComponent = pCarrier->GetComponent<HullComponent>();
    const float hullRatio = static_cast<float>(hullComponent.Health) / static_cast<float>(hullComponent.MaximumHealth);
    if (m_CurrentTier == 0 && hullRatio <= 0.66f)
    {
        EscalateTier();
    }
    else if (m_CurrentTier == 1 && hullRatio <= 0.33f)
    {
        EscalateTier();
    }
}

void Encounter::EscalateTier()
{
    if (m_CurrentTier + 1 < m_EncounterTiers.size() && m_EncounterTiers[m_CurrentTier + 1].pDeck != nullptr)
    {
        m_CurrentTier++;
        Log::Info() << "Encounter escalated to tier " << m_CurrentTier;
    }
    else
    {
        m_EncounterTiers[m_CurrentTier].pDeck->ShuffleAndReset();
    }
}

void Encounter::EvaluateEncounterResult()
{
    if (GetEncounterResult() != EncounterResult::Undecided)
    {
        return;
    }

    EntitySharedPtr pCarrier = m_pCarrier.lock();
    if (!pCarrier)
    {
        m_EncounterResult = EncounterResult::Victory;
    }

    SectorSharedPtr pSector = m_pSector.lock();
    if (!pSector)
    {
        return;
    }

    if (!pSector->GetPlayerCarrier() || !pSector->GetPlayerMech())
    {
        m_EncounterResult = EncounterResult::Defeat;
    }

    if (m_EncounterResult != EncounterResult::Undecided)
    {
        m_EncounterStats.finishedAt = std::chrono::steady_clock::now();
        m_EncounterResolvedSignal.Emit(m_EncounterResult);
        m_EncounterResolvedSignal.DisconnectAll();
    }
}

void Encounter::DrawDebugUI()
{
    if (!IsDebugUIVisible())
    {
        return;
    }

    if (ImGui::Begin("Encounter", &m_ShowDebugUI))
    {
        ImGui::Text("Time to next action: %.2f seconds", m_TimeToNextAction);
        ImGui::Text("Current tier: %d", m_CurrentTier + 1);

        if (ImGui::BeginTable("decks", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Tier");
            ImGui::TableSetupColumn("Card");
            ImGui::TableHeadersRow();

            for (size_t tier = 0; tier < m_EncounterTiers.size(); tier++)
            {
                Deck* pDeck = m_EncounterTiers[tier].pDeck.get();
                if (!pDeck)
                {
                    continue;
                }

                std::vector<std::pair<Card*, bool>> cards = pDeck->GetAllCards();
                for (auto [pCard, played] : cards)
                {
                    ImGui::BeginDisabled(played);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", static_cast<int>(tier + 1));
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", pCard->GetName().c_str());
                    ImGui::EndDisabled();
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }
}

void Encounter::OnEntityKilled(EntitySharedPtr pKilledEntity, EntitySharedPtr pKilledByEntity)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    if (pKilledByEntity == pSector->GetPlayerMech())
    {
        m_EncounterStats.kills++;
    }
}

} // namespace WingsOfSteel
