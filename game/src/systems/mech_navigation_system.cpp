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

namespace WingsOfSteel
{

void MechNavigationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const MechNavigationComponent, const MechEngineComponent, RigidBodyComponent>();

    view.each([this, delta](const auto entity, const MechNavigationComponent& mechNavigationComponent, const MechEngineComponent& mechEngineComponent, RigidBodyComponent& rigidBodyComponent) {
        const std::optional<glm::vec3>& thrustDirection = mechNavigationComponent.GetThrust();
        if (thrustDirection.has_value())
        {
            rigidBodyComponent.ApplyLinearForce(thrustDirection.value() * mechEngineComponent.linearForce);
        }

        if (mechNavigationComponent.GetAim().has_value())
        {
            TurnTowards(mechNavigationComponent.GetAim().value(), rigidBodyComponent, mechEngineComponent.torque);
        }
    });
}

} // namespace WingsOfSteel