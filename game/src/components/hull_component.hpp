#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class HullComponent : public Pandora::IComponent
{
public:
    HullComponent() {}
    ~HullComponent() {}

    // If the health of the hull is 0 or less, the ship should be destroyed.
    int32_t Health{10};

    // Thickness is essentially armor that doesn't get degraded.
    // This is used by ammo to calculate whether it punches through an entity or not.
    int32_t Thickness{10};

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["health"] = Health;
        json["thickness"] = Thickness;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        Health = DeserializeRequired<int32_t>(json, "health");
        Thickness = DeserializeOptional<int32_t>(json, "thickness", 10);
    }
};

REGISTER_COMPONENT(HullComponent, "hull")

} // namespace WingsOfSteel::TheBrightestStar