#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class PlayerControllerComponent : public IComponent
{
public:
    PlayerControllerComponent() {}
    ~PlayerControllerComponent() {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
    }

private:
    // A component must have some data.
    int m_Placeholder{ 0 };
};

REGISTER_COMPONENT(PlayerControllerComponent, "player_controller")

} // namespace WingsOfSteel