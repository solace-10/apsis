#include "systems/ammo_system.hpp"

#include <core/random.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/entity_reference_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/ammo_impact_component.hpp"
#include "components/ammo_movement_component.hpp"
#include "components/ammo_raycast_component.hpp"
#include "components/hull_component.hpp"
#include "components/shield_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel
{

void AmmoSystem::Initialize(Scene* pScene)
{
    // Initialization stub
}

void AmmoSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const TransformComponent, AmmoImpactComponent, const AmmoRaycastComponent, EntityReferenceComponent>();

    PhysicsSimulationSystem* pPhysicsSystem = GetActiveScene()->GetSystem<PhysicsSimulationSystem>();

    view.each([delta, this, pPhysicsSystem](
                  const auto entityHandle,
                  const TransformComponent& transformComponent,
                  AmmoImpactComponent& ammoImpactComponent,
                  const AmmoRaycastComponent& ammoRaycastComponent,
                  EntityReferenceComponent& entityReferenceComponent) {
        glm::vec3 startPos = transformComponent.GetTranslation();
        glm::vec3 direction = transformComponent.GetForward();
        glm::vec3 endPos = startPos + direction * ammoRaycastComponent.GetRaycastLength();

        std::vector<PhysicsSimulationSystem::RaycastResult> raycastResults = pPhysicsSystem->RaycastAll(startPos, endPos);
        if (raycastResults.empty())
        {
            GetDebugRender()->Line(startPos, endPos, Color::Yellow);
        }
        else
        {
            for (const auto& result : raycastResults)
            {
                bool hasHitTerminalEntity = false;
                bool hasHitShieldEntity = false;
                bool wasBlockedByShield = false;
                EntitySharedPtr pAmmoEntity = entityReferenceComponent.GetOwner();
                if (pAmmoEntity)
                {
                    EntitySharedPtr pHitEntity = result.pEntity;
                    if (pHitEntity)
                    {
                        hasHitShieldEntity = pHitEntity->HasComponent<ShieldComponent>();    
                        if (WasBlockedByShield(pHitEntity, result.position))
                        {
                            hasHitTerminalEntity = true;
                            wasBlockedByShield = true;
                        }

                        if (!hasHitShieldEntity && !wasBlockedByShield)
                        {
                            bool hitEntityStillAlive = true;
                            ApplyHullDamage(pAmmoEntity, pHitEntity, hitEntityStillAlive);
                            hasHitTerminalEntity = true;

                            if (!hitEntityStillAlive)
                            {
                                Game::Get()->GetSector()->RemoveEntity(pHitEntity);
                            }
                        }

                        if (hasHitTerminalEntity)
                        {
                            Game::Get()->GetSector()->RemoveEntity(pAmmoEntity);
                        }
                    }
                }

                if (hasHitTerminalEntity)
                {
                    GetDebugRender()->Line(startPos, result.position, Color::Red);
                    GetDebugRender()->Circle(result.position, glm::vec3(0.0f, 1.0f, 0.0f), Color::Red, 0.2f, 8);
                    break;
                }
                else
                {
                    GetDebugRender()->Line(startPos, endPos, Color::Yellow);
                }
            }
        }
    });

    // Advance the projectile and kill it if it has exceeded its maximum range.
    auto movementView = registry.view<TransformComponent, AmmoMovementComponent, EntityReferenceComponent>();
    movementView.each([delta, this, pPhysicsSystem](const auto entity, TransformComponent& transformComponent, AmmoMovementComponent& ammoMovementComponent, EntityReferenceComponent& entityReferenceComponent) {
        const glm::vec3 startPos = transformComponent.GetTranslation();
        const glm::vec3 direction = transformComponent.GetForward();

        const float distance = delta * ammoMovementComponent.GetSpeed();
        const float rangeAfterAdvance = ammoMovementComponent.GetRange() - distance;
        if (rangeAfterAdvance < 0.0f)
        {
            EntitySharedPtr pAmmoEntity = entityReferenceComponent.GetOwner();
            if (pAmmoEntity)
            {
                Game::Get()->GetSector()->RemoveEntity(pAmmoEntity);
            }
        }
        else
        {
            const glm::vec3 offsetPos(startPos + direction * distance);
            transformComponent.transform[3] = glm::vec4(offsetPos.x, offsetPos.y, offsetPos.z, 1.0f);
            ammoMovementComponent.SetRange(rangeAfterAdvance);
        }
    });
}

void AmmoSystem::Instantiate(EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    glm::mat4 ammoTransform = pWeaponEntity->GetComponent<TransformComponent>().transform;

    // Accuracy changes the initial forward vector of the ammo being fired.
    // Values are in degrees.
    static const std::array<float, static_cast<size_t>(WeaponAccuracy::Count)> sAccuracyModifiers{
        0.0f, // Perfect
        2.5f, // Very high
        5.0f, // High
        10.0f, // Medium
        15.0f, // Low
    };

    if (weaponComponent.m_Accuracy != WeaponAccuracy::Perfect)
    {
        const float halfModifier = sAccuracyModifiers[static_cast<size_t>(weaponComponent.m_Accuracy)] / 2.0f;
        const float deviation = Random::Get(-halfModifier, halfModifier);
        ammoTransform = glm::rotate(ammoTransform, glm::radians(deviation), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    SceneWeakPtr pWeakScene = pSector->GetWeakPtr();
    EntityBuilder::Build(
        pWeakScene,
        weaponComponent.m_Ammo,
        ammoTransform,
        [weaponComponent](EntitySharedPtr pEntity) {
            float range = weaponComponent.m_Range;

            if (pEntity->HasComponent<AmmoRaycastComponent>())
            {
                range -= pEntity->GetComponent<AmmoRaycastComponent>().GetRaycastLength();
            }

            if (pEntity->HasComponent<AmmoMovementComponent>())
            {
                pEntity->GetComponent<AmmoMovementComponent>().SetRange(range);
            }
        });
}

void AmmoSystem::ApplyHullDamage(EntitySharedPtr pAmmoEntity, EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const
{
    if (pAmmoEntity->HasComponent<AmmoImpactComponent>() && pHitEntity->HasComponent<HullComponent>())
    {
        AmmoImpactComponent& ammoImpactComponent = pAmmoEntity->GetComponent<AmmoImpactComponent>();
        HullComponent& hullComponent = pHitEntity->GetComponent<HullComponent>();

        hullComponent.Health -= ammoImpactComponent.Damage;
        ammoImpactComponent.ArmorPenetration -= hullComponent.Thickness;
        hitEntityStillAlive = (hullComponent.Health > 0.0f);
    }
    else
    {
        hitEntityStillAlive = true;
    }
}

bool AmmoSystem::WasBlockedByShield(EntitySharedPtr pHitEntity, const glm::vec3& hitPosition) const
{
    if (!pHitEntity->HasComponent<ShieldComponent>())
    {
        return false;
    }

    const ShieldComponent& shieldComponent = pHitEntity->GetComponent<ShieldComponent>();
    if (shieldComponent.CurrentState != ShieldState::Active)
    {
        return false;
    }

    // Shield only blocks in a 180 degree front arc.
    const TransformComponent& transformComponent = pHitEntity->GetComponent<TransformComponent>();
    const glm::vec3 entityPosition = transformComponent.GetTranslation();
    const glm::vec3 entityForward = transformComponent.GetForward();
    const glm::vec3 toHitPosition = glm::normalize(hitPosition - entityPosition);

    // Dot product > 0 means the hit is in the front hemisphere.
    return glm::dot(entityForward, toHitPosition) > 0.0f;
}

} // namespace WingsOfSteel
