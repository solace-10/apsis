#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

enum class ThreatCategory
{
    Interceptor, // Interceptors are a threat mechs, but they normally do not pack enough firepower to threaten a carrier.
    AntiCapital, // Any ship which can deal significant damage to carriers should be considered AntiCapital. Not just large ships, but mechs and bombers. 
    Carrier,
    Invalid
};

class ThreatComponent : public IComponent
{
public:
    ThreatComponent() {}
    ~ThreatComponent() {}

    ThreatCategory Value;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Value = Json::DeserializeEnum<ThreatCategory>(pContext, json, "value");
    }
};

REGISTER_COMPONENT(ThreatComponent, "threat")

} // namespace WingsOfSteel
