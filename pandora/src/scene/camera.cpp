#include <glm/gtc/matrix_transform.hpp>

#include "render/window.hpp"
#include "scene/camera.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

Camera::Camera(float fov, float nearPlane, float farPlane)
: m_Fov(fov)
, m_NearPlane(nearPlane)
, m_FarPlane(farPlane)
{
    CalculateProjectionMatrix();
}

Camera::~Camera()
{

}

void Camera::LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up)
{
    m_ViewMatrix = glm::lookAt(cameraPosition, targetPosition, up);
    m_CameraTarget = targetPosition;
}

void Camera::SetNearPlane(float distance)
{
    m_NearPlane = distance;
    CalculateProjectionMatrix();
}

float Camera::GetNearPlane() const
{
    return m_NearPlane;
}

void Camera::SetFarPlane(float distance)
{
    m_FarPlane = distance;
    CalculateProjectionMatrix();
}

float Camera::GetFarPlane() const
{
    return m_FarPlane;
}

void Camera::SetFieldOfView(float degrees)
{
    m_Fov = glm::radians(degrees);
    CalculateProjectionMatrix();
}

float Camera::GetFieldOfView() const
{
    return m_Fov;
}

const glm::vec3& Camera::GetTarget() const
{
    return m_CameraTarget;
}

const glm::mat4& Camera::GetViewMatrix() const
{
    return m_ViewMatrix;
}

const glm::mat4& Camera::GetProjectionMatrix() const
{
    return m_ProjectionMatrix;
}

void Camera::CalculateProjectionMatrix()
{
    const float aspectRatio = static_cast<float>(GetWindow()->GetWidth()) / static_cast<float>(GetWindow()->GetHeight());
    m_ProjectionMatrix = glm::perspective(m_Fov, aspectRatio, m_NearPlane, m_FarPlane);
}

} // namespace WingsOfSteel::Pandora