#include <glm/gtc/matrix_transform.hpp>

#include "pandora.hpp"
#include "render/window.hpp"
#include "scene/camera.hpp"

namespace WingsOfSteel::Pandora
{

Camera::Camera(float fov, float nearPlane, float farPlane)
    : m_Fov(fov)
    , m_NearPlane(nearPlane)
    , m_FarPlane(farPlane)
{
    CalculateProjectionMatrix();
}

void Camera::LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up)
{
    m_ViewMatrix = glm::lookAt(cameraPosition, targetPosition, up);
    m_CameraPosition = cameraPosition;
    m_CameraTarget = targetPosition;
}

void Camera::SetNearPlane(float distance)
{
    m_NearPlane = distance;
    CalculateProjectionMatrix();
}

void Camera::SetFarPlane(float distance)
{
    m_FarPlane = distance;
    CalculateProjectionMatrix();
}

void Camera::SetFieldOfView(float degrees)
{
    m_Fov = glm::radians(degrees);
    CalculateProjectionMatrix();
}

const glm::mat4& Camera::GetProjectionMatrix()
{
    if (m_WindowWidth != GetWindow()->GetWidth() || m_WindowHeight != GetWindow()->GetHeight())
    {
        CalculateProjectionMatrix();
    }

    return m_ProjectionMatrix;
}

void Camera::CalculateProjectionMatrix()
{
    m_WindowWidth = GetWindow()->GetWidth();
    m_WindowHeight = GetWindow()->GetHeight();
    const float aspectRatio = static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight);
    m_ProjectionMatrix = glm::perspective(m_Fov, aspectRatio, m_NearPlane, m_FarPlane);
}

} // namespace WingsOfSteel::Pandora