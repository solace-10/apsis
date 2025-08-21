#include "systems/weapon_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/hardpoint_component.hpp"
#include "components/weapon_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/ammo_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

WeaponSystem::~WeaponSystem()
{
    Pandora::Scene* pScene = Game::Get()->GetSector();
    if (pScene)
    {
        pScene->GetRegistry().on_destroy<HardpointComponent>().disconnect<&WeaponSystem::OnHardpointsDestroyed>(this);
    }
}

void WeaponSystem::Initialize(Pandora::Scene* pScene)
{
    pScene->GetRegistry().on_destroy<HardpointComponent>().connect<&WeaponSystem::OnHardpointsDestroyed>(this);
}

void WeaponSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<WeaponComponent, TransformComponent>();

    view.each([delta, this](const auto entity, WeaponComponent& weaponComponent, TransformComponent& transformComponent) {
        glm::mat4 rootWorldTransform{ 1.0f };
        EntitySharedPtr pWeaponEntity = weaponComponent.GetOwner().lock();
        bool shouldDrawFiringArc = false;
        if (pWeaponEntity)
        {
            EntitySharedPtr pParentEntity = pWeaponEntity->GetParent().lock();
            if (pParentEntity)
            {
                rootWorldTransform = pParentEntity->GetComponent<TransformComponent>().transform;
                if (pParentEntity == Game::Get()->GetSector()->GetPlayerMech())
                {
                    shouldDrawFiringArc = true;
                }
            }
        }

        glm::mat4 hardpointWorldTransform = rootWorldTransform * weaponComponent.m_AttachmentPointTransform;
        const glm::vec3 hardpointTranslation(hardpointWorldTransform[3]);
        const glm::vec3 hardpointForward(-rootWorldTransform[2]); // We negate the forward vector because our ships are -Z facing.
        const glm::vec3 hardpointUp(rootWorldTransform[1]);

        if (shouldDrawFiringArc)
        {
            DrawFiringArc(
                hardpointTranslation,
                hardpointForward,
                hardpointUp,
                weaponComponent.m_ArcMinDegrees,
                weaponComponent.m_ArcMaxDegrees,
                weaponComponent.m_Range);
        }

        transformComponent.transform = hardpointWorldTransform;

        weaponComponent.m_FireTimer = glm::max(0.0f, weaponComponent.m_FireTimer - delta);
        if (weaponComponent.m_FireTimer <= 0.0f && weaponComponent.m_WantsToFire)
        {
            FireWeapon(pWeaponEntity, weaponComponent);
        }
    });
}

void WeaponSystem::AttachWeapon(const std::string& resourcePath, Pandora::EntitySharedPtr pParentEntity, const std::string& hardpointName)
{
    Pandora::SceneWeakPtr pWeakScene = Game::Get()->GetSector()->GetWeakPtr();

    EntityBuilder::Build(
        pWeakScene,
        resourcePath,
        glm::mat4(1.0f),
        [pWeakScene, pParentEntity, hardpointName](Pandora::EntitySharedPtr pWeaponEntity)
        {
            pWeaponEntity->SetParent(pParentEntity);

            WeaponComponent& weaponComponent = pWeaponEntity->GetComponent<WeaponComponent>();
            weaponComponent.SetOwner(pWeaponEntity);

            HardpointComponent& hardpointComponent = pParentEntity->GetComponent<HardpointComponent>();
            for (auto& hardpoint : hardpointComponent.hardpoints)
            {
                if (hardpoint.m_Name == hardpointName)
                {
                    hardpoint.m_pEntity = pWeaponEntity;
                    weaponComponent.m_AttachmentPointName = hardpoint.m_Name;
                    weaponComponent.m_AttachmentPointTransform = hardpoint.m_AttachmentPointTransform;
                    weaponComponent.m_ArcMinDegrees = hardpoint.m_ArcMinDegrees;
                    weaponComponent.m_ArcMaxDegrees = hardpoint.m_ArcMaxDegrees;
                    weaponComponent.m_AngleDegrees = (hardpoint.m_ArcMaxDegrees - hardpoint.m_ArcMinDegrees) / 2.0f;
                    break;
                }
            }
        }
    );
}

void WeaponSystem::OnHardpointsDestroyed(entt::registry& registry, entt::entity entity)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    HardpointComponent& hardpointComponent = registry.get<HardpointComponent>(entity);
    for (auto& hardpoint : hardpointComponent.hardpoints)
    {
        if (hardpoint.m_pEntity)
        {
            pSector->RemoveEntity(hardpoint.m_pEntity);
            hardpoint.m_pEntity.reset();
        }
    }
}

void WeaponSystem::FireWeapon(Pandora::EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent)
{
    weaponComponent.m_FireTimer = 1.0f / weaponComponent.m_RateOfFire;

    AmmoSystem* pAmmoSystem = Pandora::GetActiveScene()->GetSystem<AmmoSystem>();
    if (pAmmoSystem)
    {
        pAmmoSystem->Instantiate(pWeaponEntity, weaponComponent);
    }
}

void WeaponSystem::DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength)
{
    using namespace Pandora;

    // Draw base forward line
    // GetDebugRender()->Line(position, position + forward * arcLength, Color::Purple);

    // Draw arc min line
    const glm::mat4 rotationMin = glm::rotate(glm::mat4(1.0f), glm::radians(arcMinDegrees), up);
    const glm::vec3 rotatedForwardMin = glm::vec3(rotationMin * glm::vec4(forward, 0.0f));
    GetDebugRender()->Line(position, position + rotatedForwardMin * arcLength, Color::Gray);

    // Draw arc max line
    const glm::mat4 rotationMax = glm::rotate(glm::mat4(1.0f), glm::radians(arcMaxDegrees), up);
    const glm::vec3 rotatedForwardMax = glm::vec3(rotationMax * glm::vec4(forward, 0.0f));
    GetDebugRender()->Line(position, position + rotatedForwardMax * arcLength, Color::Gray);

    // Draw the arc edge as connected line segments
    glm::vec3 prevPoint = position;
    for (float angle = arcMinDegrees; angle <= arcMaxDegrees; angle += 1.0f)
    {
        const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), up);
        const glm::vec3 rotatedForward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
        const glm::vec3 currentPoint = position + rotatedForward * arcLength;

        GetDebugRender()->Line(prevPoint, currentPoint, Color::Gray);
        prevPoint = currentPoint;
    }
}

} // namespace WingsOfSteel::TheBrightestStar
