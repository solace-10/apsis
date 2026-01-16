#pragma once

#include <vector>

#include <webgpu/webgpu_cpp.h>

#include <render/vertex_types.hpp>
#include <resources/resource.fwd.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class LabelSystem : public System
{
public:
    LabelSystem() = default;
    ~LabelSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void Render(wgpu::RenderPassEncoder& renderPass);

private:
    void CreateRenderPipeline();

    static constexpr size_t kMaxLabels = 1024;
    static constexpr size_t kVerticesPerQuad = 6;
    static constexpr float kQuadHalfSize = 10.0f;

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::Buffer m_VertexBuffer;
    std::vector<VertexP2C3UV> m_VertexData;
};

} // namespace WingsOfSteel
