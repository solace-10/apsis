#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class CarrierLaunchComponent : public IComponent
{
public:
    CarrierLaunchComponent() = default;
    ~CarrierLaunchComponent() = default;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {

    }

    EntityWeakPtr LaunchedBy;
    float InterpolationValue{ 0.0f };
};

REGISTER_COMPONENT(CarrierLaunchComponent, "carrier_launch")

} // namespace WingsOfSteel