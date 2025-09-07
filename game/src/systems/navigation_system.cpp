#include <limits>

#include <glm/trigonometric.hpp>

#include "systems/navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void NavigationSystem::TurnTowards(const glm::vec3& position, Pandora::RigidBodyComponent& rigidBodyComponent, float torque)
{
    const glm::vec3 aimVector = position - rigidBodyComponent.GetPosition();
    if (glm::length(aimVector) > std::numeric_limits<float>::epsilon())
    {
        const glm::vec3 aimDirection = glm::normalize(aimVector);
        const glm::vec3 forward = rigidBodyComponent.GetForwardVector();
        const glm::vec3 rightVector = rigidBodyComponent.GetRightVector();

        float angleError = glm::acos(glm::clamp(glm::dot(aimDirection, forward), -1.0f, 1.0f));
        const float dotProduct = glm::dot(aimDirection, rightVector);
        if (dotProduct < 0.0f)
        {
            angleError *= -1.0f;
        }

        float angularVelocity = rigidBodyComponent.GetAngularVelocity().y;
        static float k_p = 1.0f;
        static float k_d = 0.25f;
        float appliedTorque = (k_p * angleError - k_d * angularVelocity) * torque;
        rigidBodyComponent.ApplyAngularForce(glm::vec3(0.0f, appliedTorque, 0.0f));
    }
}

} // namespace WingsOfSteel::TheBrightestStar