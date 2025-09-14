#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

using WingID = uint32_t;

enum class WingRole
{
    Defense,
    Offense,
    Interception,
    None,
    
    Count
};

class WingComponent : public IComponent
{
public:
    WingComponent() {}
    ~WingComponent() {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["value"] = ID;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        ID = DeserializeRequired<WingID>(json, "value");
    }

    WingID ID{0};
    WingRole Role{WingRole::Defense};
};

REGISTER_COMPONENT(WingComponent, "wing")

} // namespace WingsOfSteel