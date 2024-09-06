#pragma once

#include <glm/vec2.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class CameraSystem : public Pandora::System
{
public:
    CameraSystem();
    ~CameraSystem();

    void Initialize() override;
    void Update(float delta) override;

private:
    Pandora::InputCallbackToken m_RightMouseButtonPressedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_RightMouseButtonReleasedToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_MousePositionToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    Pandora::InputCallbackToken m_MouseWheelToken{ Pandora::InputSystem::sInvalidInputCallbackToken };
    bool m_IsDragging = false;
    bool m_InputPending = true;
    glm::vec2 m_MouseDelta{ 0.0f, 0.0f };
};

} // namespace WingsOfSteel::TheBrightestStar