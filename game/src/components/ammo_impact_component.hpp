#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class AmmoImpactComponent : public IComponent
{
public:
    AmmoImpactComponent() {}
    ~AmmoImpactComponent() {}

    int32_t ArmorPenetration{0};
    int32_t Damage{10};

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        ArmorPenetration = Json::DeserializeInteger(pContext, json, "armor_penetration");
        Damage = Json::DeserializeInteger(pContext, json, "damage");
    }
};

REGISTER_COMPONENT(AmmoImpactComponent, "ammo_impact")

} // namespace WingsOfSteel