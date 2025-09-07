#include <glm/gtc/matrix_transform.hpp>

#include "entity_builder/entity_builder.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "sector/wing.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Encounter::Encounter()
{

}

Encounter::~Encounter()
{

}

void Encounter::Initialize(SectorSharedPtr pSector)
{
    m_pSector = pSector;
    RebuildTier(0);
}

void Encounter::RebuildTier(int tier)
{
    m_EncounterTiers[tier].actions.clear();
 
    EncounterWingDescription wingDescription;
    for (int i = 0; i < 5; i++)
    {
        wingDescription.entities.push_back("/entity_prefabs/raiders/interceptor.json");
    }

    EncounterAction action;
    action.wings.push_back(wingDescription);

    m_EncounterTiers[tier].actions.push_back(action);
    m_EncounterTiers[tier].currentAction = 0;
}

void Encounter::Update(float delta)
{
    m_TimeToNextAction -= delta;
    if (m_TimeToNextAction < 0.0)
    {
        m_TimeToNextAction = 30.0f;

        EncounterTier& currentTier = m_EncounterTiers[m_CurrentTier];
        InstantiateAction(currentTier.actions[currentTier.currentAction]);
        currentTier.currentAction++;

        // Have we used all the actions in this tier?
        if (m_EncounterTiers[m_CurrentTier].currentAction >= m_EncounterTiers[m_CurrentTier].actions.size())
        {
            // Go to the next tier if possible
            if (m_CurrentTier + 1 < m_EncounterTiers.size() && !m_EncounterTiers[m_CurrentTier + 1].actions.empty())
            {
                m_CurrentTier++;
            }

            RebuildTier(m_CurrentTier);
        }
    }
}

void Encounter::InstantiateAction(const Encounter::EncounterAction& action)
{
    Pandora::SceneWeakPtr pWeakScene = std::static_pointer_cast<Pandora::Scene>(m_pSector.lock());
    for (const auto& wing : action.wings)
    {
        const size_t numShips = wing.entities.size();
        for (size_t i = 0; i < numShips; i++)
        {
            const float positionZ = -15.0f * (static_cast<float>(numShips) - 1.0f) + 30.0f * static_cast<float>(i);
            EntityBuilder::Build(pWeakScene, wing.entities[i], glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 0.0f, positionZ)), [](Pandora::EntitySharedPtr pEntity){});
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar