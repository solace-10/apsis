#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

#include "systems/ai_strikecraft_controller_system.hpp"

namespace WingsOfSteel
{

class AIStrikecraftControllerComponent : public IComponent
{
public:
    AIStrikecraftControllerComponent() = default;
    ~AIStrikecraftControllerComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        if (json.contains("min_range")) Context.minRange = json["min_range"];
        if (json.contains("max_range")) Context.maxRange = json["max_range"];
        if (json.contains("firing_angle")) Context.firingAngle = json["firing_angle"];
        if (json.contains("attack_duration")) Context.attackDuration = json["attack_duration"];
        if (json.contains("break_duration")) Context.breakDuration = json["break_duration"];
    }

    StrikecraftContext Context;
};

REGISTER_COMPONENT(AIStrikecraftControllerComponent, "ai_strikecraft_controller")

} // namespace WingsOfSteel
