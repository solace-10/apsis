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

        const glm::vec2& thrustDirection = mechNavigationComponent.GetThrust();
        const glm::vec3 thrustDirectionXZ(thrustDirection.x, 0.0f, -thrustDirection.y);
        rigidBodyComponent.ApplyLinearForce(thrustDirectionXZ * mechEngineComponent.linearForce);
    });
}

} // namespace WingsOfSteel::TheBrightestStar