#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class AIStrikecraftControllerComponent;
class ShipNavigationComponent;

class AIStrikecraftControllerSystem : public Pandora::System
{
public:
    AIStrikecraftControllerSystem() = default;
    ~AIStrikecraftControllerSystem() = default;

    void Initialize(Pandora::Scene* pScene) override {}
    void Update(float delta) override;

private:
    Pandora::EntitySharedPtr AcquireTarget() const;
    void ProcessCombatState(entt::entity entity, ShipNavigationComponent& navigation, AIStrikecraftControllerComponent& controller, const Pandora::TransformComponent& transform, Pandora::EntitySharedPtr target, float delta);
    glm::vec3 CalculateInterceptPoint(const glm::vec3& shooterPos, const glm::vec3& targetPos, const glm::vec3& targetVel, float projectileSpeed) const;
    glm::vec3 GenerateBreakDirection(const glm::vec3& forward, const glm::vec3& toTarget) const;
    void UpdateWeaponSystems(entt::entity entity, const glm::vec3& targetPos, bool shouldFire);
};

} // namespace WingsOfSteel::TheBrightestStar