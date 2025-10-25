#pragma once

#include "resources/resource.fwd.hpp"
#include <render/render_pass/render_pass.hpp>
#include <render/material.hpp>
#include <resources/resource_texture_2d.hpp>
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
    void UpdateVertexBuffer();

    ResourceShaderSharedPtr m_pShader;
    ResourceTexture2DSharedPtr m_pChevron;
    MaterialUniquePtr m_ChevronMaterial;
    wgpu::Buffer m_VertexBuffer;
    wgpu::RenderPipeline m_RenderPipeline;
    bool m_Initialized = false;
    uint32_t m_VertexCount = 0;
};

} // namespace WingsOfSteel
