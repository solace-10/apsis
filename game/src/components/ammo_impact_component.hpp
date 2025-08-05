#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class AmmoImpactComponent : public Pandora::IComponent
{
public:
    AmmoImpactComponent() {}
    ~AmmoImpactComponent() {}

    int32_t ArmorPenetration{0};
    int32_t Damage{10};

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["armor_penetration"] = ArmorPenetration;
        json["damage"] = Damage;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        ArmorPenetration = DeserializeRequired<int32_t>(json, "armor_penetration");
        Damage = DeserializeRequired<int32_t>(json, "damage");
    }
};

REGISTER_COMPONENT(AmmoImpactComponent, "ammo_impact")

} // namespace WingsOfSteel::TheBrightestStar