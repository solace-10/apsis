#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include <core/log.hpp>

#include "components/ship_engine_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel
{

void ShipNavigationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const ShipNavigationComponent, const ShipEngineComponent, RigidBodyComponent>();

    view.each([this, delta](const auto entity, const ShipNavigationComponent& shipNavigationComponent, const ShipEngineComponent& shipEngineComponent, RigidBodyComponent& rigidBodyComponent) {
        if (shipNavigationComponent.GetThrust() != ShipThrust::None)
        {
            glm::vec3 force = rigidBodyComponent.GetForwardVector() * shipEngineComponent.linearForce;
            if (shipNavigationComponent.GetThrust() == ShipThrust::Backward)
            {
                force *= -0.5f;
            }

            rigidBodyComponent.ApplyLinearForce(force);
        }

        // Keep the ship within the XZ plane.
        // In particular this helps with bringing the ship into the gameplay plane after it exits the carrier.
        const float y = rigidBodyComponent.GetWorldTransform()[3].y;
        if (glm::abs(y) > 0.25f)
        {
            glm::vec3 verticalForce = glm::vec3(0.0f, -glm::sign(y), 0.0f) * shipEngineComponent.linearForce / 2.0f;
            rigidBodyComponent.ApplyLinearForce(verticalForce);
        }

        std::optional<glm::vec3> targetPosition = shipNavigationComponent.GetTarget();
        if (targetPosition.has_value())
        {
            TurnTowards(targetPosition.value(), rigidBodyComponent, shipEngineComponent.torque);
        }
    });
}

} // namespace WingsOfSteel