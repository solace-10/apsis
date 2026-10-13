#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include "resources/resource_model.hpp"
#include "scene/systems/system.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ModelVisualization);

class ModelRenderSystem : public System
{
public:
    ModelRenderSystem();
    ~ModelRenderSystem();

    void Initialize(Scene* pScene) override{};
    void Update(float delta) override{};

    void Render(wgpu::RenderPassEncoder& renderPass);

    ModelVisualization* GetVisualization() { return m_pModelVisualization.get(); }

private:
    ModelVisualizationUniquePtr m_pModelVisualization;

    struct InstanceData
    {
        ResourceModelWeakPtr pModel;
        ResourceModel::InstanceTransforms transforms;
        std::vector<std::unordered_map<std::string, float>> shaderParameters;
    };

    std::vector<InstanceData> m_InstanceData;
};

} // namespace WingsOfSteel