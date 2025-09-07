#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using ddVec3 = glm::vec3;
using ddVec3_In = const glm::vec3&;
using ddVec3_Out = glm::vec3&;

#include <debug_draw.hpp>

#include <webgpu/webgpu_cpp.h>

#include "resources/resource_shader.hpp"

namespace WingsOfSteel::Private
{

class DebugRenderImpl : public dd::RenderInterface
{
public:
    DebugRenderImpl();
    ~DebugRenderImpl();

    void Initialize();
    void Render(wgpu::RenderPassEncoder& renderPass);

    dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void* pPixels) override;
    void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override;

    void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override;
    void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override;
    void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override;

private:
    void CreateLineRenderPipeline();
    void CreateGlyphRenderPipeline();

    wgpu::Buffer m_LineVertexBuffer;
    wgpu::RenderPipeline m_LineRenderPipeline;
    ResourceShaderSharedPtr m_pUntexturedShader;

    wgpu::Buffer m_GlyphVertexBuffer;
    wgpu::RenderPipeline m_GlyphRenderPipeline;
    ResourceShaderSharedPtr m_pGlyphShader;
    wgpu::Texture m_GlyphTexture;
    wgpu::TextureView m_GlyphTextureView;

    wgpu::BindGroup m_GlyphBindGroup;
    wgpu::BindGroupLayout m_GlyphBindGroupLayout;

    struct LineData
    {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<LineData> m_LineData;

    struct GlyphData
    {
        glm::vec2 position;
        glm::vec3 color;
        glm::vec2 uv;
    };
    std::vector<GlyphData> m_GlyphData;
};

} // namespace WingsOfSteel::Private