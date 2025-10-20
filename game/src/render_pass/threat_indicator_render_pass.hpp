#pragma once

#include <render/render_pass/render_pass.hpp>
#include <resources/resource_shader.hpp>
#include <webgpu/webgpu_cpp.h>
#include <glm/vec3.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ThreatIndicatorRenderPass);
class ThreatIndicatorRenderPass : public RenderPass
{
public:
    ThreatIndicatorRenderPass();

    void Render(wgpu::CommandEncoder& encoder) override;

private:
    void CreateRenderPipeline();

    ResourceShaderSharedPtr m_pShader;
    wgpu::Buffer m_VertexBuffer;
    wgpu::RenderPipeline m_RenderPipeline;
    bool m_Initialized = false;
};

} // namespace WingsOfSteel