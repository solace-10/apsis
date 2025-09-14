#include <sstream>

#include <render/debug_render.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

#include "components/hull_component.hpp"
#include "systems/target_overlay_system.hpp"

namespace WingsOfSteel
{

void TargetOverlaySystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const HullComponent, const TransformComponent>();

    view.each([&registry](const auto entity, const HullComponent& hullComponent, const TransformComponent& transformComponent)
    {
        std::stringstream ss;
        ss << "Hull: " << hullComponent.Health;

        glm::vec3 origin(transformComponent.transform[3]);
        glm::vec3 offset(origin + glm::vec3(5.0f, 0.0f, 5.0f));
        GetDebugRender()->Line(origin, offset, Color::Wheat);
        GetDebugRender()->Label(ss.str(), offset, Color::Wheat);
    });
}

} // namespace WingsOfSteel