#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class ShipEngineComponent : public Pandora::IComponent
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

    void Deserialize(const nlohmann::json& json) override
    {
        linearForce = DeserializeOptional<float>(json, "linear_force", 0.0f);
        torque = DeserializeOptional<float>(json, "torque", 0.0f);
    }
};

REGISTER_COMPONENT(ShipEngineComponent, "ship_engine")

} // namespace WingsOfSteel::TheBrightestStar