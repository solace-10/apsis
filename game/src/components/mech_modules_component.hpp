#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class MechModulesComponent : public IComponent
{
public:
    EntityWeakPtr LeftArm;
    EntityWeakPtr RightArm;
    EntityWeakPtr LeftShoulder;
    EntityWeakPtr RightShoulder;
    EntityWeakPtr EnergyShield;
    
    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }
};

REGISTER_COMPONENT(MechModulesComponent, "mech_modules")

} // namespace WingsOfSteel
