#pragma once

#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

#include "components/weapon_component.hpp"

namespace WingsOfSteel
{

class AmmoSystem : public System
{
public:
    AmmoSystem() = default;
    ~AmmoSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void Instantiate(EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent);

private:
    void ApplyHullDamage(EntitySharedPtr pAmmoEntity, EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const;
};

} // namespace WingsOfSteel
