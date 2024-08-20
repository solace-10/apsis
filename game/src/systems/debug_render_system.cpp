#include <render/debug_render.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

#include "systems/debug_render_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void DebugRenderSystem::Update(float delta)
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const DebugRenderComponent, const TransformComponent>();

    view.each([](const auto entity, const DebugRenderComponent& debugRenderComponent, const TransformComponent& transformComponent)
    {
        if (debugRenderComponent.shape == DebugRenderShape::Circle)
        {
            GetDebugRender()->Circle(
                glm::vec3(transformComponent.transform[3]),
                glm::vec3(0.0f, 1.0f, 0.0f),
                debugRenderComponent.color,
                debugRenderComponent.radius.value_or(1.0f),
                20.0f
            );
        }
        else if (debugRenderComponent.shape == DebugRenderShape::Cone)
        {
            const float radius = debugRenderComponent.radius.value_or(1.0f);
            const glm::mat4 apex = glm::translate(transformComponent.transform, glm::vec3(radius, 0.0f, 0.0f));
            const glm::mat4 base = glm::translate(transformComponent.transform, glm::vec3(-radius * 2.0f, 0.0f, 0.0f));
            GetDebugRender()->Cone(glm::vec3(apex[3]), glm::vec3(base[3]), debugRenderComponent.color, radius, 0.0f);
        }
        else if (debugRenderComponent.shape == DebugRenderShape::Box)
        {
            auto buildPoint = [](const glm::mat4& transform, float localX, float localY, float localZ) -> glm::vec3
            {
                return glm::vec3(glm::translate(transform, glm::vec3(localX, localY, localZ))[3]);
            };

            const float halfLength = debugRenderComponent.length.value_or(1.0f) / 2.0f;
            const float halfWidth = debugRenderComponent.width.value_or(1.0f) / 2.0f;
            const float halfHeight = debugRenderComponent.height.value_or(1.0f) / 2.0f;
            const glm::vec3 points[8] = {
                buildPoint(transformComponent.transform,  halfLength,  halfHeight,  halfWidth),
                buildPoint(transformComponent.transform,  halfLength,  halfHeight, -halfWidth),
                buildPoint(transformComponent.transform,  halfLength, -halfHeight, -halfWidth),
                buildPoint(transformComponent.transform,  halfLength, -halfHeight,  halfWidth),
                buildPoint(transformComponent.transform, -halfLength,  halfHeight,  halfWidth),
                buildPoint(transformComponent.transform, -halfLength,  halfHeight, -halfWidth),
                buildPoint(transformComponent.transform, -halfLength, -halfHeight, -halfWidth),
                buildPoint(transformComponent.transform, -halfLength, -halfHeight,  halfWidth)
            };

            GetDebugRender()->Box(points, debugRenderComponent.color);
        }

        //GetDebugRender()->Label(m_pSignal->GetName(), m_pSignal->GetPosition(), Color::White);
    });
}

} // namespace WingsOfSteel::TheBrightestStar