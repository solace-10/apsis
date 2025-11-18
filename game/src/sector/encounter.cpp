#include <glm/gtc/matrix_transform.hpp>

#include <core/serialization.hpp>
#include <core/log.hpp>
#include <core/random.hpp>
#include <imgui/imgui.hpp>
#include <pandora.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>

#include "components/faction_component.hpp"
#include "components/wing_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/deck/deck.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "sector/wing.hpp"
#include "systems/carrier_system.hpp"

namespace WingsOfSteel
{

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
    GetResourceSystem()->RequestResource(encounterFile, [this, pWeakSector](ResourceSharedPtr pResource)
    {
        SectorSharedPtr pSector = pWeakSector.lock();
        if (!pSector)
        {
            return;
        }

        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (pDataStore)
        {
            for (uint32_t tier = 1; tier <= 1; tier++)
            {
                DeckUniquePtr pDeck = std::make_unique<Deck>();
                pDeck->Initialize(pDataStore.get(), tier);
                m_EncounterTiers[tier - 1] = std::move(pDeck);
            }

            SpawnCarrier();
        }
    });
}

void Encounter::SpawnCarrier()
{
    SceneWeakPtr pWeakScene = m_pSector;
    SectorWeakPtr pWeakSector = m_pSector;
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/raiders/carrier.json", glm::translate(glm::mat4(1.0f), glm::vec3(250.0f, 0.0f, 0.0f)), [pWeakSector](EntitySharedPtr pEntity){
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
    if (m_pCarrier.expired())
    {
        return;
    }

    m_TimeToNextAction -= delta;
    if (m_TimeToNextAction < 0.0)
    {
        Deck* pCurrentDeck = m_EncounterTiers[m_CurrentTier].get();
        if (pCurrentDeck->PlayNextCard())
        {
            m_TimeToNextAction = 30.0f;
        }
        else
        {
            if (m_CurrentTier + 1 < m_EncounterTiers.size())
            {
                m_CurrentTier++;
            }
            else
            {
                m_EncounterTiers[m_CurrentTier]->ShuffleAndReset();
            }
        }
    }

    DrawDebugUI();
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
                Deck* pDeck = m_EncounterTiers[tier].get();
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
                    ImGui::Text("%d", tier + 1);
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

} // namespace WingsOfSteel
