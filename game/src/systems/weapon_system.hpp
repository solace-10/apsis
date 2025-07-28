#pragma once

#include <glm/vec3.hpp>
#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

#include "components/weapon_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class WeaponSystem : public Pandora::System
{
public:
    WeaponSystem() = default;
    ~WeaponSystem() = default;

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

    void AttachWeapon(const std::string& resourcePath, Pandora::EntitySharedPtr pParentEntity, const std::string& hardpointName);

private:
    void DrawFiringArc(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float arcMinDegrees, float arcMaxDegrees, float arcLength);
    void FireWeapon(Pandora::EntitySharedPtr pWeaponEntity, WeaponComponent& weaponComponent);
};

} // namespace WingsOfSteel::TheBrightestStar
