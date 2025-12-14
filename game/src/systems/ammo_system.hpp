#pragma once

#include <glm/vec3.hpp>

#include <core/signal.hpp>
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

    Signal<EntitySharedPtr, EntitySharedPtr>& GetEntityKilledSignal() { return m_EntityKilledSignal; }

private:
    void ApplyHullDamage(EntitySharedPtr pAmmoEntity, EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const;
    bool WasBlockedByShield(EntitySharedPtr pHitEntity, const glm::vec3& hitPosition) const;

    Signal<EntitySharedPtr, EntitySharedPtr> m_EntityKilledSignal;
};

} // namespace WingsOfSteel
