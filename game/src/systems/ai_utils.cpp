#include <cassert>

#include <glm/vec3.hpp>

#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/hardpoint_component.hpp"
#include "components/threat_component.hpp"
#include "components/weapon_component.hpp"
#include "systems/ai_utils.hpp"

namespace WingsOfSteel
{

EntitySharedPtr AIUtils::AcquireTarget(EntitySharedPtr pAcquiringEntity, TargetPriorityOrder targetPriorityOrder, std::optional<std::string> priorityThreatType)
{
    glm::vec3 startPosition{ 0.0f };
    if (targetPriorityOrder == TargetPriorityOrder::Closest)
    {
        startPosition = pAcquiringEntity->GetComponent<TransformComponent>().GetTranslation();
    }
    else
    {
        assert(false); // Not implemented yet.
        return nullptr;
    }

    EntitySharedPtr pAcquiredTarget;
    float closestDistanceSquared = FLT_MAX;

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto threatsView = registry.view<const TransformComponent, const ThreatComponent>();
    threatsView.each([&startPosition, &closestDistanceSquared, &pAcquiredTarget](const auto entityHandle, const TransformComponent& transformComponent, const ThreatComponent& threatComponent) {
        const glm::vec3 entityPosition = transformComponent.GetTranslation();
        const glm::vec3 positionDelta = entityPosition - startPosition;
        const float distanceSquared = glm::dot(positionDelta, positionDelta);

        if (distanceSquared < closestDistanceSquared)
        {
            closestDistanceSquared = distanceSquared;
            pAcquiredTarget = GetActiveScene()->GetEntity(entityHandle);
        }
    });

    return pAcquiredTarget;
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

} // namespace WingsOfSteel
