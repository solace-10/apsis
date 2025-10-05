#pragma once

#include <array>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <scene/components/transform_component.hpp>
#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

#include "components/faction_component.hpp"
#include "components/weapon_component.hpp"

namespace WingsOfSteel
{

class WeaponSystem : public System
{
public:
    WeaponSystem() = default;
    ~WeaponSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void AttachWeapon(const std::string& resourcePath, EntitySharedPtr pParentEntity, const std::string& hardpointName, bool automatedTargeting);

private:
    void OnHardpointsDestroyed(entt::registry& registry, entt::entity entity);

    void DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength);
    void DrawFiringLine(const glm::vec3& position, const glm::vec3& forward, float angle, float lineLength);
    void FireWeapon(EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent);
    void AcquireTarget(float delta, const glm::mat4& hardpointWorldTransform, WeaponComponent& weaponComponent, const FactionComponent& factionComponent);
    void TurnTowardsTarget(float delta, const glm::mat4& hardpointWorldTransform, WeaponComponent& weaponComponent, TransformComponent& TransformComponent);
    void UpdateTransform(const glm::mat4& hardpointWorldTransform, const WeaponComponent& weaponComponent, TransformComponent& transformComponent);
    void UpdateFiring(float delta, const glm::mat4& hardpointWorldTransform, EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent, const FactionComponent& factionComponent);
    void BuildPotentialTargetList();

    struct PotentialTarget
    {
        EntityHandle entityHandle;
        glm::vec3 position;
        glm::vec3 velocity;
    };
    std::array<std::vector<PotentialTarget>, 2> m_PotentialTargets; // One list of targets per faction.
};

} // namespace WingsOfSteel
