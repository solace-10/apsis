#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class MechEngineComponent : public IComponent
{
public:
    float linearForce{ 0.0f };
    float torque{ 0.0f };

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["linear_force"] = linearForce;
        json["torque"] = torque;
        return json;
    }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        linearForce = Json::DeserializeFloat(pContext, json, "linear_force", 0.0f);
        torque = Json::DeserializeFloat(pContext, json, "torque", 0.0f);
    }
};

REGISTER_COMPONENT(MechEngineComponent, "mech_engine")

} // namespace WingsOfSteel