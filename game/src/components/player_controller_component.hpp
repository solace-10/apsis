#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class PlayerControllerComponent : public Pandora::IComponent
{
public:
    PlayerControllerComponent() {}
    ~PlayerControllerComponent() {}

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["placeholder"] = m_Placeholder;
        return json;
    }

    void Deserialize(const nlohmann::json& json) override
    {
        m_Placeholder = DeserializeOptional<int>(json, "placeholder", 0);
    }

private:
    // A component must have some data.
    int m_Placeholder{ 0 };
};

PANDORA_REGISTER_COMPONENT(PlayerControllerComponent, "player_controller")

} // namespace WingsOfSteel::TheBrightestStar