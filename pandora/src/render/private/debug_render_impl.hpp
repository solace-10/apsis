#pragma once

#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using ddVec3 = glm::vec3;
using ddVec3_In = const glm::vec3&;
using ddVec3_Out = glm::vec3&;

// Debug-draw configuration. These MUST be defined before the first include of debug_draw.hpp:
// the library resolves its DEBUG_DRAW_MAX_* / DEBUG_DRAW_OVERFLOWED defaults inside its
// declaration include guard, so this is the earliest (and only) point they can be overridden.
//
// A physics wireframe of the heightfield emits (Width * Length * 2) triangles * 3 edges of
// lines every frame (~240k lines for the default 200x200 landscape), far exceeding the
// library's default 32768-line queue. This is a development-only debug view, so we trade
// memory (~25 MB for the line queue) for the ability to draw the whole collision mesh at once.
#define DEBUG_DRAW_MAX_LINES 524288

// Route queue overflow through a throttled log instead of the library default (fprintf to
// stderr, which fires once per dropped primitive — hundreds of thousands of times per frame —
// and floods/hangs the browser console on web). Definition lives in debug_render_impl.cpp.
namespace WingsOfSteel::Private { void DebugDrawOverflowHandler(const char* pMessage); }
#define DEBUG_DRAW_OVERFLOWED(message) ::WingsOfSteel::Private::DebugDrawOverflowHandler(message)

#include <debug_draw.hpp>

#include <webgpu/webgpu_cpp.h>

#include "render/rendersystem.hpp"
#include "render/vertex_types.hpp"
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
    uint64_t m_LineVertexBufferSize{ 0 }; // Capacity of m_LineVertexBuffer in bytes; grown on demand.
    wgpu::RenderPipeline m_LineRenderPipeline;
    ResourceShaderSharedPtr m_pUntexturedShader;

    wgpu::Buffer m_GlyphVertexBuffer;
    wgpu::RenderPipeline m_GlyphRenderPipeline;
    ResourceShaderSharedPtr m_pGlyphShader;
    wgpu::Texture m_GlyphTexture;
    wgpu::TextureView m_GlyphTextureView;

    wgpu::BindGroup m_GlyphBindGroup;
    wgpu::BindGroupLayout m_GlyphBindGroupLayout;

    using LineData = VertexP3C3;
    std::vector<LineData> m_LineData;

    using GlyphData = VertexP2C3UV;
    std::vector<GlyphData> m_GlyphData;
};

} // namespace WingsOfSteel::Private