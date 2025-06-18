#include "scene/systems/model_render_system.hpp"

#include "pandora.hpp"
#include "scene/components/model_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::Pandora
{

ModelRenderSystem::ModelRenderSystem()
{
}

ModelRenderSystem::~ModelRenderSystem()
{
}

void ModelRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<ModelComponent, TransformComponent>();

    view.each([&renderPass](const auto entity, ModelComponent& modelComponent, TransformComponent& transformComponent) {
        ResourceModel* pResourceModel = modelComponent.GetModel();
        if (pResourceModel)
        {
            pResourceModel->Render(renderPass, transformComponent.transform);
        }
    });
}

} // namespace WingsOfSteel::Pandora