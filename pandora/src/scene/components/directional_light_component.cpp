#include "scene/components/directional_light_component.hpp"
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace WingsOfSteel
{

void DirectionalLightComponent::SetAngle(float angleInDegrees)
{
    m_Angle = angleInDegrees;
    CalculateNormalizedDirection();
}

void DirectionalLightComponent::SetPitch(float pitchInDegrees)
{
    m_Pitch = pitchInDegrees;
    CalculateNormalizedDirection();
}

void DirectionalLightComponent::SetDirection(const glm::vec3& direction)
{
    m_Direction = glm::normalize(direction);

    // The angle and pitch are back-computed rather than left stale. LightingSystem caches all
    // three, and the Lighting debug window reads the angles back to seed its sliders, so a
    // direction set this way has to describe itself the same way one set from the sliders does.
    //
    // This is the exact inverse of CalculateNormalizedDirection() below, which builds
    // (-sinAngle*cosPitch, sinPitch, -cosAngle*cosPitch): negating x and z recovers the pair
    // (sinAngle*cosPitch, cosAngle*cosPitch), whose atan2 is the angle for any pitch strictly
    // between straight down and straight up.
    m_Pitch = glm::degrees(std::asin(glm::clamp(m_Direction.y, -1.0f, 1.0f)));
    m_Angle = glm::degrees(std::atan2(-m_Direction.x, -m_Direction.z));
    if (m_Angle < 0.0f)
    {
        m_Angle += 360.0f;
    }
}

void DirectionalLightComponent::CalculateNormalizedDirection()
{
    // Convert angle and pitch to direction vector
    // Angle rotates around Y axis (horizontal), pitch rotates up/down
    const float angleInRadians = glm::radians(m_Angle);
    const float pitchInRadians = glm::radians(m_Pitch);
    const float cosAngle = glm::cos(angleInRadians);
    const float sinAngle = glm::sin(angleInRadians);
    const float cosPitch = glm::cos(pitchInRadians);
    const float sinPitch = glm::sin(pitchInRadians);

    // Calculate direction vector pointing TO the light (opposite of ray direction)
    // This is used in shaders as: NdotL = dot(normal, lightDirection)
    m_Direction.x = -sinAngle * cosPitch;
    m_Direction.y = sinPitch; // Positive for light coming from above
    m_Direction.z = -cosAngle * cosPitch;

    // Normalize to ensure unit vector
    m_Direction = glm::normalize(m_Direction);
}

} // namespace WingsOfSteel
