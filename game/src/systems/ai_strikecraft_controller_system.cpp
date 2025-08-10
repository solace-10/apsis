#include <pandora.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/ai_strikecraft_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/weapon_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/ai_strikecraft_controller_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void AIStrikecraftControllerSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();

    auto navigationView = registry.view<ShipNavigationComponent, const AIStrikecraftControllerComponent>();
    navigationView.each([this](const auto entity, ShipNavigationComponent& navigationComponent, const AIStrikecraftControllerComponent& controllerComponent) {
        EntitySharedPtr pTargetEntity = controllerComponent.GetTarget();
        if (!pTargetEntity)
        {
            pTargetEntity = AcquireTarget();
        }

        if (pTargetEntity)
        {
            const glm::vec3 targetWorldPos = pTargetEntity->GetComponent<TransformComponent>().GetTranslation();
            navigationComponent.SetTarget(targetWorldPos);
            navigationComponent.SetThrust(ShipThrust::Forward);
        }
        else
        {
            navigationComponent.ClearTarget();
            navigationComponent.SetThrust(ShipThrust::None);
        }
    });

    /*
    auto weaponsView = registry.view<WeaponComponent>();
    weaponsView.each([this, targetWorldPos](const auto entity, WeaponComponent& weaponComponent) {
        EntitySharedPtr pParentShip;
        EntitySharedPtr pOwnerEntity = weaponComponent.GetOwner().lock();
        if (pOwnerEntity)
        {
            pParentShip = pOwnerEntity->GetParent().lock();
            if (pParentShip && pParentShip->HasComponent<PlayerControllerComponent>())
            {
                weaponComponent.m_Target = targetWorldPos;

                auto it = m_WeaponActivations.find(weaponComponent.m_AttachmentPointName);
                if (it == m_WeaponActivations.cend())
                {
                    weaponComponent.m_WantsToFire = false;
                }
                else
                {
                    weaponComponent.m_WantsToFire = it->second;
                }
            }
        }
    });
    */
}

Pandora::EntitySharedPtr AIStrikecraftControllerSystem::AcquireTarget() const
{
    return Game::Get()->GetSector()->GetPlayerMech();
}

} // namespace WingsOfSteel::TheBrightestStar