#include <glm/gtx/rotate_vector.hpp>

#include "systems/weapon_system.hpp"

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/faction_component.hpp"
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
    BuildPotentialTargetList();

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<WeaponComponent, TransformComponent, const FactionComponent>();
    view.each([delta, this](const auto entity, WeaponComponent& weaponComponent, TransformComponent& transformComponent, const FactionComponent& factionComponent) {
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

        if (shouldDrawFiringArc || true)
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

        AcquireTarget(delta, hardpointWorldTransform, weaponComponent, factionComponent);
        TurnTowardsTarget(delta, hardpointWorldTransform, weaponComponent, transformComponent);
        UpdateTransform(hardpointWorldTransform, weaponComponent, transformComponent);
        UpdateFiring(delta, hardpointWorldTransform, pWeaponEntity, weaponComponent);
    });
}

void WeaponSystem::AttachWeapon(const std::string& resourcePath, EntitySharedPtr pParentEntity, const std::string& hardpointName, bool automatedTargeting)
{
    SceneWeakPtr pWeakScene = Game::Get()->GetSector()->GetWeakPtr();

    EntityBuilder::Build(
        pWeakScene,
        resourcePath,
        glm::mat4(1.0f),
        [pWeakScene, pParentEntity, hardpointName, automatedTargeting](EntitySharedPtr pWeaponEntity)
        {
            pWeaponEntity->SetParent(pParentEntity);

            WeaponComponent& weaponComponent = pWeaponEntity->GetComponent<WeaponComponent>();
            weaponComponent.SetOwner(pWeaponEntity);

            if (pParentEntity->HasComponent<FactionComponent>())
            {
                pWeaponEntity->AddComponent<FactionComponent>(pParentEntity->GetComponent<FactionComponent>().Value);
            }

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
                    weaponComponent.m_AutomatedTargeting = automatedTargeting;
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

void WeaponSystem::AcquireTarget(float delta, const glm::mat4& hardpointWorldTransform, WeaponComponent& weaponComponent, const FactionComponent& factionComponent)
{
    if (!weaponComponent.m_AutomatedTargeting)
    {
        return;
    }

    const glm::vec3 hardpointPosition(hardpointWorldTransform[3]); 
    weaponComponent.m_TargetPosition = std::nullopt;
    weaponComponent.m_TargetVelocity = std::nullopt;
    const std::vector<PotentialTarget>& potentialTargets = m_PotentialTargets[static_cast<size_t>(factionComponent.Value)];
    for (const PotentialTarget& potentialTarget : potentialTargets)
    {
        const float distance = glm::distance(hardpointPosition, potentialTarget.position);
        if (distance > weaponComponent.m_Range)
        {
            continue;
        }

        // Check if we are inside the firing arc.
        const glm::vec3 hardpointForward(hardpointWorldTransform[2]);
        const glm::vec3 hardpointUp(hardpointWorldTransform[1]);

        // Project vectors to XZ plane for horizontal angle calculation.
        const glm::vec3 hardpointForwardXZ = glm::normalize(glm::vec3(hardpointForward.x, 0.0f, hardpointForward.z));
        glm::vec3 directionToTargetXZ = glm::normalize(potentialTarget.position - hardpointPosition);
        directionToTargetXZ = glm::normalize(glm::vec3(directionToTargetXZ.x, 0.0f, directionToTargetXZ.z));

        // Calculate angle to target.
        float cosAngle = glm::dot(hardpointForwardXZ, directionToTargetXZ);
        cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);
        float angleToTarget = glm::degrees(glm::acos(cosAngle));

        const glm::vec3 cross = glm::cross(hardpointForwardXZ, directionToTargetXZ);
        if (cross.y < 0.0f)
        {
            angleToTarget = -angleToTarget;
        }

        // If we're not within the firing arc, skip this target.
        if (angleToTarget < weaponComponent.m_ArcMinDegrees || angleToTarget > weaponComponent.m_ArcMaxDegrees)
        {
            continue;
        }

        weaponComponent.m_TargetPosition = potentialTarget.position;
        break;
    }
}

void WeaponSystem::TurnTowardsTarget(float delta, const glm::mat4& hardpointWorldTransform, WeaponComponent& weaponComponent, TransformComponent& transformComponent)
{
    if (!weaponComponent.m_TargetPosition.has_value())
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

    const glm::vec3 target = weaponComponent.m_TargetPosition.value();
    glm::vec3 directionToTargetXZ = glm::normalize(target - hardpointTranslation);
    directionToTargetXZ = glm::normalize(glm::vec3(directionToTargetXZ.x, 0.0f, directionToTargetXZ.z));

    // Calculate the angle between hardpoint forward and direction to target
    float cosAngle = glm::dot(hardpointForwardXZ, directionToTargetXZ);
    cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);
    float angleToTarget = glm::degrees(glm::acos(cosAngle));

    // Determine the sign of the angle using cross product
    glm::vec3 cross = glm::cross(hardpointForwardXZ, directionToTargetXZ);
    if (cross.y < 0.0f)
    {
        angleToTarget = -angleToTarget;
    }

    // Calculate the maximum turn amount for this frame
    const float turnRate = 45.0f;
    const float maxTurn = turnRate * delta;

    // Calculate the angle difference from current weapon angle to target angle
    const float targetAngle = angleToTarget;
    const float angleDifference = targetAngle - weaponComponent.m_AngleDegrees;

    // Apply turn, but don't overshoot
    float newAngle;
    if (glm::abs(angleDifference) <= maxTurn)
    {
        newAngle = targetAngle;
    }
    else
    {
        newAngle = weaponComponent.m_AngleDegrees + glm::sign(angleDifference) * maxTurn;
    }

    weaponComponent.m_AngleDegrees = glm::clamp(
        newAngle,
        weaponComponent.m_ArcMinDegrees,
        weaponComponent.m_ArcMaxDegrees
    );

    GetDebugRender()->Line(hardpointTranslation, target, Color::Cyan);
}

void WeaponSystem::UpdateTransform(const glm::mat4& hardpointWorldTransform, const WeaponComponent& weaponComponent, TransformComponent& transformComponent)
{
    const glm::mat4 weaponRotation = glm::rotate(glm::mat4(1.0f), glm::radians(weaponComponent.m_AngleDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    transformComponent.transform = hardpointWorldTransform * weaponRotation;
}

void WeaponSystem::UpdateFiring(float delta, const glm::mat4& hardpointWorldTransform, EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent)
{
    if (weaponComponent.m_AutomatedTargeting)
    {
        
    }

    weaponComponent.m_FireTimer = glm::max(0.0f, weaponComponent.m_FireTimer - delta);
    if (weaponComponent.m_FireTimer <= 0.0f && weaponComponent.m_WantsToFire)
    {
        FireWeapon(pWeaponEntity, weaponComponent);
    }
}

void WeaponSystem::BuildPotentialTargetList()
{
    for (auto& targetList : m_PotentialTargets)
    {
        targetList.clear();
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const RigidBodyComponent, const FactionComponent>();

    view.each([this](const auto entityHandle, const RigidBodyComponent& rigidBodyComponent, const FactionComponent& factionComponent) {
        PotentialTarget potentialTarget{
            .entityHandle = entityHandle,
            .position = rigidBodyComponent.GetWorldTransform()[3],
            .velocity = rigidBodyComponent.GetLinearVelocity()
        };

        Faction otherFaction = Faction::Allied;
        if (factionComponent.Value == Faction::Allied)
        {
            otherFaction = Faction::Hostile;
        }
        
        m_PotentialTargets[static_cast<size_t>(otherFaction)].push_back(potentialTarget);
    });
}

} // namespace WingsOfSteel
