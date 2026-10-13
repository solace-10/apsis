#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "pandora.hpp"
#include "render/window.hpp"
#include "scene/camera.hpp"

namespace WingsOfSteel
{

Camera::Camera(float fov, float nearPlane, float farPlane)
    : m_Fov(glm::radians(fov))
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
    m_ProjectionMatrix = glm::perspectiveZO(m_Fov, aspectRatio, m_NearPlane, m_FarPlane);
}

glm::vec3 Camera::ScreenToWorld(const glm::vec2& mousePos, uint32_t windowWidth, uint32_t windowHeight, float depth) const
{
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    const float x = (2.0f * mousePos.x) / static_cast<float>(windowWidth) - 1.0f;
    const float y = 1.0f - (2.0f * mousePos.y) / static_cast<float>(windowHeight);

    // Create a point in clip space at the specified depth
    const glm::vec4 clipCoords(x, y, depth, 1.0f);

    // Use camera-relative matrices for precision (see WorldToScreen for details).
    glm::mat4 viewRotation = m_ViewMatrix;
    viewRotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    const glm::mat4 viewProj = m_ProjectionMatrix * viewRotation;
    const glm::mat4 invViewProj = glm::inverse(viewProj);

    // Transform from clip space to camera-relative world space
    glm::vec4 relativeCoords = invViewProj * clipCoords;

    // Perform perspective division
    if (relativeCoords.w != 0.0f)
    {
        relativeCoords /= relativeCoords.w;
    }

    // Add camera position back to get world space
    return glm::vec3(relativeCoords) + m_CameraPosition;
}

glm::vec3 Camera::WorldToScreen(const glm::vec3& worldPos, uint32_t windowWidth, uint32_t windowHeight) const
{
    // Use camera-relative position to avoid float precision issues with large world coordinates.
    // The view matrix bakes camera translation into dot products, causing catastrophic cancellation
    // when world positions are large (e.g. geosynchronous orbit at ~42,000 km).
    // By subtracting the camera position first, all values stay small and well-conditioned.
    const glm::vec3 relativePos = worldPos - m_CameraPosition;
    glm::mat4 viewRotation = m_ViewMatrix;
    viewRotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    const glm::mat4 viewProj = m_ProjectionMatrix * viewRotation;
    const glm::vec4 clipCoords = viewProj * glm::vec4(relativePos, 1.0f);

    // Perform perspective division to get normalized device coordinates
    glm::vec3 ndc;
    if (clipCoords.w != 0.0f)
    {
        ndc = glm::vec3(clipCoords) / clipCoords.w;
    }
    else
    {
        ndc = glm::vec3(clipCoords);
    }

    // Convert NDC to screen coordinates
    // x: [-1, 1] -> [0, windowWidth]
    // y: [-1, 1] -> [windowHeight, 0] (flip Y for screen space where 0,0 is top-left)
    // z: keep in [0, 1] range for depth
    const float screenX = (ndc.x + 1.0f) * 0.5f * static_cast<float>(windowWidth);
    const float screenY = (1.0f - ndc.y) * 0.5f * static_cast<float>(windowHeight);

    return glm::vec3(screenX, screenY, ndc.z);
}

} // namespace WingsOfSteel
