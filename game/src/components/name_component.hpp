#pragma once

#include <string>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class NameComponent : public Pandora::IComponent
{
public:
    NameComponent() {}
    NameComponent(const std::string& name) : Value(name) {}
    ~NameComponent() {}

    std::string Value;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["value"] = Value;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        Value = DeserializeRequired<std::string>(json, "value");
    }
};

PANDORA_REGISTER_COMPONENT(NameComponent, "name")

} // namespace WingsOfSteel::TheBrightestStar