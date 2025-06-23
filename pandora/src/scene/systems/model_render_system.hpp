#pragma once

#include <webgpu/webgpu_cpp.h>

#include "scene/systems/system.hpp"

namespace WingsOfSteel::Pandora
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
};

} // namespace WingsOfSteel::Pandora