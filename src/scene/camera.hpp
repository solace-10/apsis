#pragma once

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace WingsOfSteel
{

class Camera
{
public:
    Camera(float fov, float nearPlane, float farPlane);
    ~Camera() {}

    void LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up);

    void SetNearPlane(float distance);
    float GetNearPlane() const;
    void SetFarPlane(float distance);
    float GetFarPlane() const;
    // Degrees, both ways. The constructor takes them too; only m_Fov is kept in radians, which
    // is what glm::perspectiveZO() wants.
    void SetFieldOfView(float degrees);
    float GetFieldOfView() const;

    const glm::vec3& GetPosition() const;
    const glm::vec3& GetTarget() const;
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix(); // Not constant as it can recalculate the projection matrix if the window size has changed.

    // Convert mouse screen coordinates to world space coordinates
    // mousePos: screen coordinates (0,0 at top-left, width/height at bottom-right)
    // windowWidth/Height: current window dimensions
    // Returns: world space position at the specified depth (default 0.0f for near plane)
    glm::vec3 ScreenToWorld(const glm::vec2& mousePos, uint32_t windowWidth, uint32_t windowHeight, float depth = 0.0f) const;

    // Convert world space coordinates to screen space coordinates
    // worldPos: position in world space
    // windowWidth/Height: current window dimensions
    // Returns: screen coordinates (0,0 at top-left, width/height at bottom-right), z component contains depth in [0,1] range
    glm::vec3 WorldToScreen(const glm::vec3& worldPos, uint32_t windowWidth, uint32_t windowHeight) const;

private:
    void CalculateProjectionMatrix();

    float m_Fov = 0.785398f; // Radians. 45 degrees; every constructor overwrites it.
    float m_NearPlane = 0.01f;
    float m_FarPlane = 100.0f;
    glm::mat4 m_ViewMatrix{ 1.0f };
    glm::mat4 m_ProjectionMatrix{ 1.0f };
    glm::vec3 m_CameraPosition{ 0.0f };
    glm::vec3 m_CameraTarget{ 0.0f };
    uint32_t m_WindowWidth{ 0 };
    uint32_t m_WindowHeight{ 0 };
};

inline float Camera::GetNearPlane() const
{
    return m_NearPlane;
}

inline float Camera::GetFarPlane() const
{
    return m_FarPlane;
}

inline float Camera::GetFieldOfView() const
{
    return glm::degrees(m_Fov);
}

inline const glm::vec3& Camera::GetPosition() const
{
    return m_CameraPosition;
}

inline const glm::vec3& Camera::GetTarget() const
{
    return m_CameraTarget;
}

inline const glm::mat4& Camera::GetViewMatrix() const
{
    return m_ViewMatrix;
}

} // namespace WingsOfSteel
