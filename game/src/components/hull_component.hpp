#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class HullComponent : public IComponent
{
public:
    HullComponent() {}
    ~HullComponent() {}

    // If the health of the hull is 0 or less, the ship should be destroyed.
    int32_t Health{ 10 };
    int32_t MaximumHealth{ 10 };

    // Thickness is essentially armor that doesn't get degraded.
    // This is used by ammo to calculate whether it punches through an entity or not.
    int32_t Thickness{10};

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        MaximumHealth = Health = Json::DeserializeInteger(pContext, json, "health");
        Thickness = Json::DeserializeInteger(pContext, json, "thickness", 10);
    }
};

REGISTER_COMPONENT(HullComponent, "hull")

} // namespace WingsOfSteel
