#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class ThreatIndicatorComponent : public IComponent
{
public:
    ThreatIndicatorComponent() {}
    ThreatIndicatorComponent(const std::string& name) : Value(name) {}
    ~ThreatIndicatorComponent() {}

    std::string Value;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Value = Json::DeserializeString(pContext, json, "value");
    }
};

REGISTER_COMPONENT(ThreatIndicatorComponent, "threat_indicator")

} // namespace WingsOfSteel