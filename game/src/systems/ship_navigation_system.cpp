#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

#include "components/ship_navigation_component.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void ShipNavigationSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const ShipNavigationComponent, TransformComponent>();

    // TEMP: Modify the transform directly until we Bullet3 and rigid bodies available.
    view.each([delta](const auto entity, const ShipNavigationComponent& shipNavigationComponent, TransformComponent& transformComponent)
    {
        if (shipNavigationComponent.GetThrust() > 0.0f)
        {
            const float shipMaximumSpeed = 20.0f * shipNavigationComponent.GetThrust() * delta;
            transformComponent.transform = glm::translate(transformComponent.transform, glm::vec3(shipMaximumSpeed, 0.0f, 0.0f));
        }

        if (shipNavigationComponent.GetSteerCommand() != ShipSteerCommand::None)
        {
            const float dir = (shipNavigationComponent.GetSteerCommand() == ShipSteerCommand::Port) ? -1.0f : 1.0f;
            const float shipMaximumAngularSpeed = 1.0f * dir * delta;
            transformComponent.transform = glm::rotate(transformComponent.transform, shipMaximumAngularSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    });
}

} // namespace WingsOfSteel::TheBrightestStar