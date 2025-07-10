#pragma once

#include <glm/vec2.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

#include "components/ship_navigation_component.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class PlayerControllerSystem : public Pandora::System
{
public:
    PlayerControllerSystem(){};
    ~PlayerControllerSystem();

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

private:
    Pandora::InputCallbackToken m_ForwardButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_ForwardButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_LeftButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_LeftButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_RightButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_RightButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_MousePositionToken{ Pandora::InputSystem::sInvalidInputCallbackToken };

    ShipSteer m_Steer{ ShipSteer::None };
    ShipThrust m_Thrust{ ShipThrust::None };
    glm::vec2 m_MousePosition{ 0.0f };
};

} // namespace WingsOfSteel::TheBrightestStar