#pragma once

#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

#include "components/weapon_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class AmmoSystem : public Pandora::System
{
public:
    AmmoSystem() = default;
    ~AmmoSystem() = default;

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

    void Instantiate(Pandora::EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent);
};

} // namespace WingsOfSteel::TheBrightestStar
