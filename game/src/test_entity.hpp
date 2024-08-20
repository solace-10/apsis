#pragma once

#include <webgpu/webgpu_cpp.h>

#include <resources/resource.fwd.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(TestEntity);
class TestEntity : public Pandora::Entity
{
public:
    TestEntity(Pandora::Scene* pScene);
    ~TestEntity();

    void OnAddedToScene();
    void Render(wgpu::RenderPassEncoder renderPass) override;

private:
    void CreateRenderPipeline();

    Pandora::ResourceModelSharedPtr m_pModel;

    wgpu::Buffer m_VertexBuffer;
    wgpu::RenderPipeline m_RenderPipeline;
    Pandora::ResourceShaderSharedPtr m_pShader;
};

} // namespace WingsOfSteel::TheBrightestStar