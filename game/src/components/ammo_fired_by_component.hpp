#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

// This component is automatically added by AmmoSystem when new projectiles are generated.
// It allows us to link back to the weapon which fired it, as well as the weapon's owner (e.g. mech or ship).
class AmmoFiredByComponent : public IComponent
{
public:
    AmmoFiredByComponent() = default;
    ~AmmoFiredByComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override {}

    EntityWeakPtr Weapon;
    EntityWeakPtr WeaponOwner;
};

REGISTER_COMPONENT(AmmoFiredByComponent, "ammo_fired_by")

} // namespace WingsOfSteel
