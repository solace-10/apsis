#pragma once

#include <glm/vec3.hpp>

using ddVec3 = glm::vec3;
using ddVec3_In = const glm::vec3&;
using ddVec3_Out = glm::vec3&;

#include <debug_draw.hpp>

#include <webgpu/webgpu_cpp.h>

#include "resources/resource_shader.hpp"

namespace WingsOfSteel::Pandora::Private
{

class DebugRenderImpl : public dd::RenderInterface
{
public:
    DebugRenderImpl();
    ~DebugRenderImpl();

    void Initialize();
    void Render(wgpu::RenderPassEncoder& renderPass);

    // virtual dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels) override;
    // virtual void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override;

    void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override;
    void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override;
    // virtual void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override;

private:
    void CreateLineRenderPipeline();

    wgpu::Buffer m_LineVertexBuffer;
    wgpu::RenderPipeline m_LineRenderPipeline;
    ResourceShaderSharedPtr m_pUntexturedShader;

    struct LineData
    {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<LineData> m_LineData;
};

} // namespace WingsOfSteel::Pandora::Private