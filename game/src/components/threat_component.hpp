#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class ThreatComponent : public IComponent
{
public:
    ThreatComponent() {}
    ThreatComponent(const std::string& name) : Value(name) {}
    ~ThreatComponent() {}

    std::string Value;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Value = Json::DeserializeString(pContext, json, "value");
    }
};

REGISTER_COMPONENT(ThreatComponent, "threat")

} // namespace WingsOfSteel
