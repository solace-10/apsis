#include <core/color.hpp>
#include <render/debug_render.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

#include "systems/space_object_render_system.hpp"
#include "components/space_object_component.hpp"

namespace WingsOfSteel
{

SpaceObjectRenderSystem::SpaceObjectRenderSystem()
{
}

SpaceObjectRenderSystem::~SpaceObjectRenderSystem()
{
}

void SpaceObjectRenderSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const SpaceObjectComponent, const TransformComponent>();

    DebugRender* pDebugRender = GetDebugRender();

    view.each([pDebugRender](const auto entity, const SpaceObjectComponent& spaceObjectComponent, const TransformComponent& transformComponent)
    {
        // Extract position from transform matrix (translation is in column 3)
        glm::vec3 position = glm::vec3(transformComponent.transform[3]);

        // Draw a small cube at the space object's position
        // Size of 100km to be visible at orbital scale
        constexpr float kBoxSize = 100.0f;
        pDebugRender->Box(position, Color::Cyan, kBoxSize, kBoxSize, kBoxSize);
    });
}

} // namespace WingsOfSteel
