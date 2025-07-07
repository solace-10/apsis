#include <glm/gtc/matrix_inverse.hpp>
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
    m_ViewMatrix = glm::lookAtRH(cameraPosition, targetPosition, up);
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
    m_ProjectionMatrix = glm::perspectiveRH_ZO(m_Fov, aspectRatio, m_NearPlane, m_FarPlane);
}

glm::vec3 Camera::ScreenToWorld(const glm::vec2& mousePos, uint32_t windowWidth, uint32_t windowHeight, float depth) const
{
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float x = (2.0f * mousePos.x) / static_cast<float>(windowWidth) - 1.0f;
    float y = 1.0f - (2.0f * mousePos.y) / static_cast<float>(windowHeight);

    // Create a point in clip space at the specified depth
    glm::vec4 clipCoords(x, y, depth, 1.0f);

    // Get the inverse of the view-projection matrix
    glm::mat4 viewProj = m_ProjectionMatrix * m_ViewMatrix;
    glm::mat4 invViewProj = glm::inverse(viewProj);

    // Transform from clip space to world space
    glm::vec4 worldCoords = invViewProj * clipCoords;

    // Perform perspective division
    if (worldCoords.w != 0.0f)
    {
        worldCoords /= worldCoords.w;
    }

    return glm::vec3(worldCoords);
}

} // namespace WingsOfSteel::Pandora