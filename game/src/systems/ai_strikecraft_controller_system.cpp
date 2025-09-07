#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <render/debug_render.hpp>
#include <core/random.hpp>

#include "components/ai_strikecraft_controller_component.hpp"
#include "components/hardpoint_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/weapon_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/ai_strikecraft_controller_system.hpp"

namespace WingsOfSteel
{

void AIStrikecraftControllerSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto controllerView = registry.view<ShipNavigationComponent, AIStrikecraftControllerComponent, TransformComponent>();
    controllerView.each([this, delta](const auto entity, ShipNavigationComponent& navigationComponent, AIStrikecraftControllerComponent& controllerComponent, const TransformComponent& transform) {
        EntitySharedPtr pTargetEntity = controllerComponent.GetTarget();
        if (!pTargetEntity)
        {
            pTargetEntity = AcquireTarget();
            controllerComponent.SetTarget(pTargetEntity);
        }

        if (pTargetEntity)
        {
            controllerComponent.UpdateTimers(delta);
            ProcessCombatState(entity, navigationComponent, controllerComponent, transform, pTargetEntity, delta);
        }
        else
        {
            navigationComponent.ClearTarget();
            navigationComponent.SetThrust(ShipThrust::None);
        }
    });

}

void AIStrikecraftControllerSystem::ProcessCombatState(entt::entity entity, ShipNavigationComponent& navigation, AIStrikecraftControllerComponent& controller, const TransformComponent& transform, EntitySharedPtr target, float delta)
{
    const glm::vec3 myPos = transform.GetTranslation();
    const glm::vec3 targetPos = target->GetComponent<TransformComponent>().GetTranslation();
    const glm::vec3 toTarget = targetPos - myPos;
    const float distanceToTarget = glm::length(toTarget);
    const glm::vec3 forward = transform.GetForward();
    const float angleToTarget = glm::degrees(glm::acos(glm::clamp(glm::dot(glm::normalize(toTarget), forward), -1.0f, 1.0f)));
    
    controller.SetLastTargetPosition(targetPos);
    
    switch (controller.GetState())
    {
        case AIStrikecraftState::APPROACH:
        {
            glm::vec3 interceptPoint = CalculateInterceptPoint(myPos, targetPos, glm::vec3(0.0f), 1000.0f);
            navigation.SetTarget(interceptPoint);
            navigation.SetThrust(ShipThrust::Forward);
            
            if (distanceToTarget <= controller.GetOptimalRange())
            {
                controller.SetState(AIStrikecraftState::ATTACK);
            }
            
            UpdateWeaponSystems(entity, targetPos, false);
            break;
        }
        
        case AIStrikecraftState::ATTACK:
        {
            glm::vec3 interceptPoint = CalculateInterceptPoint(myPos, targetPos, glm::vec3(0.0f), 1000.0f);
            navigation.SetTarget(interceptPoint);
            navigation.SetThrust(ShipThrust::Forward);
            
            const bool shouldFire = controller.ShouldFire(distanceToTarget, angleToTarget);
            UpdateWeaponSystems(entity, targetPos, shouldFire);
            
            if (controller.ShouldChangeState() || distanceToTarget < controller.GetMinRange())
            {
                glm::vec3 breakDir = GenerateBreakDirection(forward, glm::normalize(toTarget));
                controller.SetBreakDirection(breakDir);
                controller.SetState(AIStrikecraftState::BREAK);
            }
            break;
        }
        
        case AIStrikecraftState::BREAK:
        {
            // Break away from the target. The actual distance doesn't matter, as we'll keep moving towards it for `break_duration`. 
            glm::vec3 breakTarget = myPos + controller.GetBreakDirection() * 100.0f;
            navigation.SetTarget(breakTarget);
            navigation.SetThrust(ShipThrust::Forward);
            
            UpdateWeaponSystems(entity, targetPos, false);
            
            if (controller.ShouldChangeState())
            {
                if (Random::Get(0.0f, 1.0f) < 0.8f)
                {
                    controller.SetState(AIStrikecraftState::APPROACH);
                }
                else
                {
                    const glm::vec3 repositionDirection = glm::normalize(glm::vec3(Random::Get(-1.0f, 1.0f), 0.0f, Random::Get(-1.0f, 1.0f)));
                    const glm::vec3 repositionOffset = repositionDirection * controller.GetOptimalRange();
                    controller.SetRepositionTarget(targetPos + repositionOffset);
                    controller.SetState(AIStrikecraftState::REPOSITION);
                }
            }
            break;
        }
        
        case AIStrikecraftState::REPOSITION:
        {
            const glm::vec3& repositionTarget = controller.GetRepositionTarget();
            navigation.SetTarget(repositionTarget);
            navigation.SetThrust(ShipThrust::Forward);
            
            UpdateWeaponSystems(entity, targetPos, false);

            const float repositionGoalRadius = 20.0f;
            const float distanceToReposition = glm::length(repositionTarget - myPos);
            if (distanceToReposition < repositionGoalRadius)
            {
                controller.SetState(AIStrikecraftState::APPROACH);
            }
            break;
        }
    }
}

glm::vec3 AIStrikecraftControllerSystem::CalculateInterceptPoint(const glm::vec3& shooterPos, const glm::vec3& targetPos, const glm::vec3& targetVel, float projectileSpeed) const
{
    const glm::vec3 toTarget = targetPos - shooterPos;
    const float distance = glm::length(toTarget);
    const float timeToIntercept = distance / projectileSpeed;
    return targetPos + targetVel * timeToIntercept;
}

glm::vec3 AIStrikecraftControllerSystem::GenerateBreakDirection(const glm::vec3& forward, const glm::vec3& toTarget) const
{
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));    
    const float rightComponent = Random::Get(-1.0f, 1.0f);
    const float forwardComponent = Random::Get(-0.5f, 0.5f);
    const glm::vec3 breakDir = right * rightComponent + forward * forwardComponent;
    return glm::normalize(breakDir);
}

void AIStrikecraftControllerSystem::UpdateWeaponSystems(entt::entity shipEntity, const glm::vec3& targetPos, bool shouldFire)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    
    auto hardpointView = registry.view<HardpointComponent>();
    hardpointView.each([&registry, shipEntity, targetPos, shouldFire](const auto entity, HardpointComponent& hardpointComponent) {
        if (entity == shipEntity)
        {
            for (const auto& hardpoint : hardpointComponent.hardpoints)
            {
                if (hardpoint.m_pEntity && hardpoint.m_pEntity->HasComponent<WeaponComponent>())
                {
                    WeaponComponent& weaponComponent = hardpoint.m_pEntity->GetComponent<WeaponComponent>();
                    weaponComponent.m_Target = targetPos;
                    weaponComponent.m_WantsToFire = shouldFire;
                }
            }
        }
    });
}

EntitySharedPtr AIStrikecraftControllerSystem::AcquireTarget() const
{
    return Game::Get()->GetSector()->GetPlayerMech();
}

} // namespace WingsOfSteel