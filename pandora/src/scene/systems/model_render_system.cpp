#include "scene/systems/model_render_system.hpp"

#include "debug_visualization/model_visualization.hpp"
#include "pandora.hpp"
#include "resources/resource_model.hpp"
#include "scene/components/model_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

ModelRenderSystem::ModelRenderSystem()
{
    m_pModelVisualization = std::make_unique<ModelVisualization>();
    m_InstanceData.resize(64);
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

    for (auto& instanceData : m_InstanceData)
    {
        instanceData.transforms.clear();
    }

    view.each([this](const auto entity, ModelComponent& modelComponent, TransformComponent& transformComponent) {
        ResourceModelSharedPtr pResourceModel = modelComponent.GetModel();
        if (pResourceModel)
        {
            // Grow the storage for instance data if required.
            const size_t idx = static_cast<size_t>(pResourceModel->GetId());
            if (idx >= m_InstanceData.size())
            {
                m_InstanceData.resize(idx + 32);
            }

            m_InstanceData[idx].pModel = pResourceModel;
            m_InstanceData[idx].transforms.push_back(transformComponent.transform);
        }
    });

    for (auto& instanceData : m_InstanceData)
    {
        ResourceModelSharedPtr pModel = instanceData.pModel.lock();
        if (pModel && !instanceData.transforms.empty())
        {
            pModel->Render(renderPass, instanceData.transforms);
        }
    }
}

} // namespace WingsOfSteel