#include <numeric>

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include <core/log.hpp>

#include "components/mech_engine_component.hpp"
#include "components/mech_navigation_component.hpp"
#include "systems/mech_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void MechNavigationSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const MechNavigationComponent, const MechEngineComponent, RigidBodyComponent>();

    view.each([delta](const auto entity, const MechNavigationComponent& mechNavigationComponent, const MechEngineComponent& mechEngineComponent, RigidBodyComponent& rigidBodyComponent) {

        const glm::vec3& thrustDirection = mechNavigationComponent.GetThrust();
        rigidBodyComponent.ApplyLinearForce(thrustDirection * mechEngineComponent.linearForce);

        const glm::vec3 aimVector = mechNavigationComponent.GetAim() - rigidBodyComponent.GetPosition();
        if (glm::length(aimVector) > std::numeric_limits<float>::epsilon())
        {
            const glm::vec3 aimDirection = glm::normalize(aimVector);
            const glm::vec3 forward = rigidBodyComponent.GetForwardVector();
            const glm::vec3 rightVector = rigidBodyComponent.GetRightVector();

            const float angleError = glm::acos(glm::clamp(glm::dot(aimDirection, forward), -1.0f, 1.0f));
            const float dotProduct = glm::dot(aimDirection, rightVector);
            if (dotProduct > 0.0f)
            {
                angleError *= -1.0f;
            }

            float angularVelocity = rigidBodyComponent.GetAngularVelocity().y;
            static float k_p = 1.0f;
            static float k_d = 0.25f;
            float torque = (k_p * angleError - k_d * angularVelocity) * mechEngineComponent.torque;
            rigidBodyComponent.ApplyAngularForce(glm::vec3(0.0f, torque, 0.0f));
        }
    });
}

} // namespace WingsOfSteel::TheBrightestStar