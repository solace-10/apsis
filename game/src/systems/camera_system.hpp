#pragma once

#include <glm/vec3.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class CameraSystem : public Pandora::System
{
public:
    CameraSystem();
    ~CameraSystem();

    void Initialize(Pandora::Scene* pScene) override;
    void Update(float delta) override;

    // Convert mouse screen coordinates to world space coordinates on the XZ plane (Y = 0)
    // mousePos: screen coordinates (0,0 at top-left, width/height at bottom-right)
    // Returns: world space position on the XZ plane, or (0,0,0) if no active camera or no intersection
    glm::vec3 MouseToWorld(const glm::vec2& mousePos) const;

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