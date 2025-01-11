#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

class Camera
{
public:
    Camera(float fov, float nearPlane, float farPlane);
    ~Camera();

    void LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up);
    
    void SetNearPlane(float distance);
    float GetNearPlane() const;
    void SetFarPlane(float distance);
    float GetFarPlane() const;
    void SetFieldOfView(float degrees);
    float GetFieldOfView() const;
    
    const glm::vec3& GetTarget() const;
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix(); // Not constant as it can recalculate the projection matrix if the window size has changed.

private:
    void CalculateProjectionMatrix();

    float m_Fov = 45.0f;
    float m_NearPlane = 0.01f;
    float m_FarPlane = 100.0f;
    glm::mat4 m_ViewMatrix{ 1.0f };
    glm::mat4 m_ProjectionMatrix{ 1.0f };
    glm::vec3 m_CameraTarget{ 0.0f };
    uint32_t m_WindowWidth{ 0 };
    uint32_t m_WindowHeight{ 0 };
};

} // namespace WingsOfSteel::Pandora