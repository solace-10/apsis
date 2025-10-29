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

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        ID = Json::DeserializeUnsignedInteger(pContext, json, "value");
    }

    WingID ID{0};
    WingRole Role{WingRole::Defense};
};

REGISTER_COMPONENT(WingComponent, "wing")

} // namespace WingsOfSteel