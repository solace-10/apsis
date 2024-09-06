#pragma once

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

#include "components/ship_navigation_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class PlayerControllerSystem : public Pandora::System
{
public:
    PlayerControllerSystem() {};
    ~PlayerControllerSystem();

    void Initialize() override;
    void Update(float delta) override;

private:
    Pandora::InputCallbackToken m_ForwardButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_ForwardButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_LeftButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_LeftButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_RightButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_RightButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };

    ShipSteerCommand m_SteerCommand{ ShipSteerCommand::None };
    float m_Thrust{ 0.0f };
};

} // namespace WingsOfSteel::TheBrightestStar