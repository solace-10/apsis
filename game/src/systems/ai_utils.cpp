#include <cassert>

#include <glm/vec3.hpp>

#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/faction_component.hpp"
#include "components/hardpoint_component.hpp"
#include "components/threat_component.hpp"
#include "components/weapon_component.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "systems/ai_utils.hpp"

namespace WingsOfSteel
{

EntitySharedPtr AIUtils::AcquireTarget(EntitySharedPtr pAcquiringEntity, const std::vector<ThreatCategory>& targetCategoryOrder, TargetRangeOrder targetRangeOrder)
{
    if (!pAcquiringEntity->HasComponent<FactionComponent>())
    {
        return nullptr;
    }

    const Faction acquiringEntityFaction = pAcquiringEntity->GetComponent<FactionComponent>().Value;
    const Faction targetFaction = GetOppositeFaction(acquiringEntityFaction);
    
    glm::vec3 startPosition{ 0.0f };
    if (targetRangeOrder == TargetRangeOrder::Closest)
    {
        startPosition = pAcquiringEntity->GetComponent<TransformComponent>().GetTranslation();
    }
    else if (targetRangeOrder == TargetRangeOrder::ClosestToCarrier)
    {
        EntitySharedPtr pCarrier = GetCarrier(acquiringEntityFaction);
        if (pCarrier && pCarrier->HasComponent<TransformComponent>())
        {
            startPosition = pCarrier->GetComponent<TransformComponent>().GetTranslation();
        }
        else
        {
            return nullptr;
        }
    }

    struct AcquiredTarget
    {
        EntityHandle entityHandle{ 0 };
        size_t threatIndex{ 0 };
        float distanceSquared{ FLT_MAX };
    };
    std::optional<AcquiredTarget> acquiredTarget;

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto threatsView = registry.view<const TransformComponent, const ThreatComponent, const FactionComponent>();
    threatsView.each([targetFaction, &startPosition, &acquiredTarget, targetCategoryOrder](const auto entityHandle, const TransformComponent& transformComponent, const ThreatComponent& threatComponent, const FactionComponent& factionComponent) {
        if (targetFaction != factionComponent.Value)
        {
            return;
        }
        
        const glm::vec3 entityPosition = transformComponent.GetTranslation();
        const glm::vec3 positionDelta = entityPosition - startPosition;
        const float distanceSquared = glm::dot(positionDelta, positionDelta);

        std::optional<size_t> threatIndex = GetThreatIndex(targetCategoryOrder, threatComponent.Value);
        if (!threatIndex.has_value())
        {
            return;
        }

        if (!acquiredTarget.has_value())
        {
            acquiredTarget = AcquiredTarget{
                .entityHandle = entityHandle,
                .threatIndex = threatIndex.value(),
                .distanceSquared = distanceSquared
            };
        }
        else
        {
            // This is a confusing bit of logic. We receive the threat category order as a vector, with the highest priority being at the start of the vector.
            // e.g. [ ThreatCategory::AntiCapital, ThreatCategory::Interceptor, ThreatCategory::Carrier ] means that AntiCapital is our highest priority.
            // By storing the index, we can quickly identify if the new threat has a higher priority than the one we previously acquired.
            // However, due to the order of categories in the vector, the index 0 has the highest priority.

            const size_t newThreatIndex = threatIndex.value();
            const size_t existingThreatIndex = acquiredTarget.value().threatIndex;
            if (newThreatIndex > existingThreatIndex)
            {
                // The new threat has a lower priority than the one we already have, don't do anything.
                return;
            }
            else if (newThreatIndex < existingThreatIndex)
            {
                // The new threat has a higher priority than the one we had acquired, switch to the new threat.
                acquiredTarget = AcquiredTarget{
                    .entityHandle = entityHandle,
                    .threatIndex = newThreatIndex,
                    .distanceSquared = distanceSquared
                };
            }
            else
            {
                // The new threat has the same priority as the one we had acquired. Prioritise the closest one.
                if (distanceSquared < acquiredTarget.value().distanceSquared)
                {
                    acquiredTarget = AcquiredTarget{
                        .entityHandle = entityHandle,
                        .threatIndex = newThreatIndex,
                        .distanceSquared = distanceSquared
                    };
                }
            }
        }
    });

    if (acquiredTarget.has_value())
    {
        return GetActiveScene()->GetEntity(acquiredTarget.value().entityHandle);
    }
    else
    {
        return nullptr;
    }
}

float AIUtils::CalculateOptimalRange(EntitySharedPtr pMechEntity)
{
    if (!pMechEntity->HasComponent<HardpointComponent>())
    {
        return 0.0f;
    }

    std::optional<float> optimalRange;
    HardpointComponent& hardpointComponent = pMechEntity->GetComponent<HardpointComponent>();
    for (auto& hardpoint : hardpointComponent.hardpoints)
    {
        if (!hardpoint.m_pEntity->HasComponent<WeaponComponent>())
        {
            continue;
        }

        WeaponComponent& weaponComponent = hardpoint.m_pEntity->GetComponent<WeaponComponent>();
        if (!optimalRange.has_value() || weaponComponent.m_Range > optimalRange.value())
        {
            optimalRange = weaponComponent.m_Range;
        }
    }

    return optimalRange.value_or(0.0f);
}

Faction AIUtils::GetOppositeFaction(Faction faction)
{
    if (faction == Faction::Allied)
    {
        return Faction::Hostile;
    }
    else if (faction == Faction::Hostile)
    {
        return Faction::Allied;
    }
    else
    {
        assert(false); // Not supported.
        return Faction::Hostile;
    }
}

std::optional<size_t> AIUtils::GetThreatIndex(const std::vector<ThreatCategory>& targetCategoryOrder, ThreatCategory targetCategory)
{
    for (size_t index = 0; index < targetCategoryOrder.size(); index++)
    {
        if (targetCategoryOrder[index] == targetCategory)
        {
            return index;
        }
    }

    return std::nullopt;
}

EntitySharedPtr AIUtils::GetCarrier(Faction faction)
{
    Sector* pSector = static_cast<Sector*>(GetActiveScene());
    if (faction == Faction::Allied)
    {
        return pSector->GetPlayerCarrier();
    }
    else
    {
        Encounter* pEncounter = pSector->GetEncounter();
        return pEncounter ? pEncounter->GetCarrier() : nullptr;
    }
}

} // namespace WingsOfSteel
