#include "systems/ammo_system.hpp"

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
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void AmmoSystem::Initialize(Pandora::Scene* pScene)
{
    // Initialization stub
}

void AmmoSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const TransformComponent, AmmoImpactComponent, const AmmoRaycastComponent, EntityReferenceComponent>();

    PhysicsSimulationSystem* pPhysicsSystem = GetActiveScene()->GetSystem<PhysicsSimulationSystem>();

    view.each([delta, this, pPhysicsSystem](
        const auto entity, 
        const TransformComponent& transformComponent, 
        AmmoImpactComponent& ammoImpactComponent,
        const AmmoRaycastComponent& ammoRaycastComponent, 
        EntityReferenceComponent& entityReferenceComponent) {
        
        glm::vec3 startPos = transformComponent.GetTranslation();
        glm::vec3 direction = -transformComponent.GetForward();
        glm::vec3 endPos = startPos + direction * ammoRaycastComponent.GetRaycastLength();

        std::optional<PhysicsSimulationSystem::RaycastResult> raycastResult = pPhysicsSystem->Raycast(startPos, endPos);
        if (raycastResult.has_value())
        {
            const auto& result = raycastResult.value();
            GetDebugRender()->Line(
                startPos,
                endPos,
                Color::Red
            );

            GetDebugRender()->Circle(
                result.position,
                glm::vec3(0.0f, 1.0f, 0.0f),
                Color::Red,
                0.2f,
                8
            );

            EntitySharedPtr pAmmoEntity = entityReferenceComponent.GetOwner();
            if (pAmmoEntity)
            {
                // TODO: Calculate penetration on hit.
                EntitySharedPtr pHitEntity = result.pEntity;
                if (pHitEntity)
                {
                    bool hitEntityStillAlive = true;
                    ApplyHullDamage(pAmmoEntity, pHitEntity, hitEntityStillAlive);

                    if (!hitEntityStillAlive)
                    {
                        Game::Get()->GetSector()->RemoveEntity(pHitEntity);
                    }
                }

                // TODO: Handle armor penetration.
                if (ammoImpactComponent.ArmorPenetration <= 0)
                {
                    Game::Get()->GetSector()->RemoveEntity(pAmmoEntity);
                } 
            }
        }
        else
        {
            GetDebugRender()->Line(
                startPos,
                endPos,
                Color::Yellow
            );
        }
    });

    // Advance the projectile and kill it if it has exceeded its maximum range.
    auto movementView = registry.view<TransformComponent, AmmoMovementComponent, EntityReferenceComponent>();
    movementView.each([delta, this, pPhysicsSystem](const auto entity, TransformComponent& transformComponent, AmmoMovementComponent& ammoMovementComponent, EntityReferenceComponent& entityReferenceComponent) {
        
        const glm::vec3 startPos = transformComponent.GetTranslation();
        const glm::vec3 direction = -transformComponent.GetForward();

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

void AmmoSystem::Instantiate(Pandora::EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent)
{
    using namespace Pandora;

    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    glm::mat4 rootWorldTransform{ 1.0f };
    EntitySharedPtr pParentEntity = pWeaponEntity->GetParent().lock();
    if (pParentEntity)
    {
        rootWorldTransform = pParentEntity->GetComponent<TransformComponent>().transform;
    }
    const glm::mat4 hardpointWorldTransform = rootWorldTransform * weaponComponent.m_AttachmentPointTransform;
    // TODO: This should include the muzzle transform.

    Pandora::SceneWeakPtr pWeakScene = pSector->GetWeakPtr();
    EntityBuilder::Build(
        pWeakScene,
        weaponComponent.m_Ammo,
        hardpointWorldTransform,
        [weaponComponent](Pandora::EntitySharedPtr pEntity)
        {
            float range = weaponComponent.m_Range;

            if (pEntity->HasComponent<AmmoRaycastComponent>())
            {
                range -= pEntity->GetComponent<AmmoRaycastComponent>().GetRaycastLength();    
            }

            if (pEntity->HasComponent<AmmoMovementComponent>())
            {
                pEntity->GetComponent<AmmoMovementComponent>().SetRange(range);
            }
        }
    );
}

void AmmoSystem::ApplyHullDamage(Pandora::EntitySharedPtr pAmmoEntity, Pandora::EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const
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

} // namespace WingsOfSteel::TheBrightestStar
