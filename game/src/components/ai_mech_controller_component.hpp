
#pragma once

#include <glm/glm.hpp>

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

#include "systems/ai_mech_controller_system.hpp"

namespace WingsOfSteel
{

class AIMechControllerComponent : public IComponent
{
public:
    AIMechControllerComponent() {}
    ~AIMechControllerComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    MechOffenseContext OffenseContext;
    MechDefenseContext DefenseContext;
    MechNavigationContext NavigationContext;
};

REGISTER_COMPONENT(AIMechControllerComponent, "ai_mech_controller")

} // namespace WingsOfSteel
