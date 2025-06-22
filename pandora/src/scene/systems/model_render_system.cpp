#include "scene/systems/model_render_system.hpp"

#include "debug_visualization/model_visualization.hpp"
#include "pandora.hpp"
#include "scene/components/model_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::Pandora
{

ModelRenderSystem::ModelRenderSystem()
{
    m_pModelVisualization = std::make_unique<ModelVisualization>();
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

    view.each([this, &renderPass](const auto entity, ModelComponent& modelComponent, TransformComponent& transformComponent) {
        ResourceModel* pResourceModel = modelComponent.GetModel();
        if (pResourceModel)
        {
            const glm::mat4& worldTransform(transformComponent.transform);
            pResourceModel->Render(renderPass, worldTransform);
            m_pModelVisualization->Render(pResourceModel, worldTransform);
        }
    });
}

} // namespace WingsOfSteel::Pandora