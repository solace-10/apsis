#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class NameComponent : public IComponent
{
public:
    NameComponent() {}
    NameComponent(const std::string& name) : Value(name) {}
    ~NameComponent() {}

    std::string Value;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Value = Json::DeserializeString(pContext, json, "value");
    }
};

REGISTER_COMPONENT(NameComponent, "name")

} // namespace WingsOfSteel