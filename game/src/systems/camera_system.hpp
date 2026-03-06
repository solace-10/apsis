#pragma once

#include <glm/vec3.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class CameraSystem : public System
{
public:
    CameraSystem() = default;
    ~CameraSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    // Convert mouse screen coordinates to world space coordinates on the XZ plane (Y = 0)
    // mousePos: screen coordinates (0,0 at top-left, width/height at bottom-right)
    // Returns: world space position on the XZ plane, or (0,0,0) if no active camera or no intersection
    glm::vec3 MouseToWorld(const glm::vec2& mousePos) const;

private:

    InputCallbackToken m_RightMouseButtonPressedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_RightMouseButtonReleasedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_MousePositionToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_MouseWheelToken{ InputSystem::sInvalidInputCallbackToken };
    bool m_IsDragging{ false };
    bool m_InputPending{ false };
    glm::vec2 m_MouseDelta{ 0.0f, 0.0f };
    float m_ScrollDelta{ 0.0f };

    // Smoothed input velocities (radians per second)
    glm::vec2 m_SmoothedInputVelocity{ 0.0f };

    // EMA smoothing factor (0 = no smoothing, 1 = instant)
    static constexpr float m_InputSmoothingFactor{ 0.3f };

    // Input velocities for momentum (radians per second)
    float m_OrbitAngleInputVelocity{ 0.0f };
    float m_PitchInputVelocity{ 0.0f };
    static constexpr float m_MomentumDecay{ 0.85f };
};

} // namespace WingsOfSteel
