#include <glm/gtx/rotate_vector.hpp>

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

namespace WingsOfSteel
{

WeaponSystem::~WeaponSystem()
{
    Scene* pScene = Game::Get()->GetSector();
    if (pScene)
    {
        pScene->GetRegistry().on_destroy<HardpointComponent>().disconnect<&WeaponSystem::OnHardpointsDestroyed>(this);
    }
}

void WeaponSystem::Initialize(Scene* pScene)
{
    pScene->GetRegistry().on_destroy<HardpointComponent>().connect<&WeaponSystem::OnHardpointsDestroyed>(this);
}

void WeaponSystem::Update(float delta)
{
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

        const glm::mat4 hardpointWorldTransform = rootWorldTransform * weaponComponent.m_AttachmentPointTransform;
        const glm::vec3 hardpointTranslation(hardpointWorldTransform[3]);
        const glm::vec3 hardpointForward(hardpointWorldTransform[2]);
        const glm::vec3 hardpointUp(hardpointWorldTransform[1]);

        if (shouldDrawFiringArc)
        {
            DrawFiringArc(
                hardpointTranslation,
                hardpointForward,
                hardpointUp,
                weaponComponent.m_ArcMinDegrees,
                weaponComponent.m_ArcMaxDegrees,
                weaponComponent.m_Range);

            DrawFiringLine(
                hardpointTranslation, 
                hardpointForward, 
                weaponComponent.m_AngleDegrees, 
                weaponComponent.m_Range
            );
        }

        TurnTowardsTarget(delta, hardpointWorldTransform, weaponComponent, pWeaponEntity);

        transformComponent.transform = hardpointWorldTransform;

        weaponComponent.m_FireTimer = glm::max(0.0f, weaponComponent.m_FireTimer - delta);
        if (weaponComponent.m_FireTimer <= 0.0f && weaponComponent.m_WantsToFire)
        {
            FireWeapon(pWeaponEntity, weaponComponent);
        }
    });
}

void WeaponSystem::AttachWeapon(const std::string& resourcePath, EntitySharedPtr pParentEntity, const std::string& hardpointName)
{
    SceneWeakPtr pWeakScene = Game::Get()->GetSector()->GetWeakPtr();

    EntityBuilder::Build(
        pWeakScene,
        resourcePath,
        glm::mat4(1.0f),
        [pWeakScene, pParentEntity, hardpointName](EntitySharedPtr pWeaponEntity)
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
                    weaponComponent.m_AngleDegrees = (hardpoint.m_ArcMinDegrees + hardpoint.m_ArcMaxDegrees) / 2.0f;
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

void WeaponSystem::FireWeapon(EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent)
{
    weaponComponent.m_FireTimer = 1.0f / weaponComponent.m_RateOfFire;

    AmmoSystem* pAmmoSystem = GetActiveScene()->GetSystem<AmmoSystem>();
    if (pAmmoSystem)
    {
        pAmmoSystem->Instantiate(pWeaponEntity, weaponComponent);
    }
}

void WeaponSystem::DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength)
{
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

void WeaponSystem::DrawFiringLine(const glm::vec3& position, const glm::vec3& forward, float angle, float lineLength)
{
    const glm::mat4 localRotationTransform = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 aimForward = glm::vec3(localRotationTransform * glm::vec4(forward, 0.0f));
    GetDebugRender()->Line(position, position + aimForward * lineLength, Color::Red);
}

void WeaponSystem::TurnTowardsTarget(float delta, const glm::mat4& hardpointWorldTransform, WeaponComponent& weaponComponent, EntitySharedPtr pWeaponEntity)
{
    if (!weaponComponent.m_Target.has_value())
    {
        return;
    }

    // Don't bother doing the turning logic if this is a fixed weapon.
    const float firingArc = glm::abs(weaponComponent.m_ArcMaxDegrees - weaponComponent.m_ArcMinDegrees);
    if (firingArc <= std::numeric_limits<float>::epsilon())
    {
        return;
    }

    const glm::vec3 hardpointTranslation(hardpointWorldTransform[3]);
    glm::vec3 hardpointForwardXZ(hardpointWorldTransform[2]);
    hardpointForwardXZ = glm::normalize(glm::vec3(hardpointForwardXZ.x, 0.0f, hardpointForwardXZ.z));

    const glm::vec3 target = weaponComponent.m_Target.value();
    const glm::vec3 directionToTarget = glm::normalize(target - hardpointTranslation);
    const glm::vec3 weaponForward = glm::rotate(hardpointForwardXZ, glm::radians(weaponComponent.m_AngleDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 weaponRight(-weaponForward.z, 0.0f, weaponForward.x); // Safe to do as weaponForward is known to be (x, 0, z).
    const float turnDirection = -glm::sign(glm::dot(weaponRight, directionToTarget));
    const float turnRate = 10.0f;
    float turnSpeed = turnRate * delta;

    // Slow down as the angle approaches the target.
    float alignment = glm::dot(weaponForward, directionToTarget);
    if (alignment > 0.9999f) turnSpeed *= (1.0f - alignment) * 10000.0f;

    weaponComponent.m_AngleDegrees = glm::clamp(
        weaponComponent.m_AngleDegrees + turnDirection * turnSpeed,
        weaponComponent.m_ArcMinDegrees,
        weaponComponent.m_ArcMaxDegrees
    );
}

} // namespace WingsOfSteel
