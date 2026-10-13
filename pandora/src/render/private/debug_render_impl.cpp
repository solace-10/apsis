#include <array>

#include "render/private/debug_render_impl.hpp"

#define DEBUG_DRAW_IMPLEMENTATION
#include <debug_draw.hpp>

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"

namespace WingsOfSteel::Private
{

// Referenced by the DEBUG_DRAW_OVERFLOWED macro (see debug_render_impl.hpp). The macro fires
// once per dropped primitive, which can be hundreds of thousands of times per frame once a
// debug queue overflows, so we throttle hard to avoid flooding the log/console.
void DebugDrawOverflowHandler(const char* pMessage)
{
    static uint64_t s_count = 0;
    if ((s_count++ % 100000) == 0)
    {
        Log::Warning() << "[DebugDraw] " << pMessage << " (further overflow messages throttled)";
    }
}

DebugRenderImpl::DebugRenderImpl()
{
}

DebugRenderImpl::~DebugRenderImpl()
{
    dd::shutdown();
}

void DebugRenderImpl::Initialize()
{
    dd::initialize(this);

    GetResourceSystem()->RequestResource("/shaders/debug_render_untextured.wgsl", [this](ResourceSharedPtr pResource) {
        m_pUntexturedShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateLineRenderPipeline();
    });

    wgpu::Device pDevice = GetRenderSystem()->GetDevice();
    wgpu::BufferDescriptor linesBufferDescriptor{
        .label = "Debug render vertex buffer (lines)",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(LineData)
    };
    m_LineVertexBuffer = pDevice.CreateBuffer(&linesBufferDescriptor);
    m_LineVertexBufferSize = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(LineData);
    m_LineData.reserve(DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    GetResourceSystem()->RequestResource("/shaders/debug_render_glyph.wgsl", [this](ResourceSharedPtr pResource) {
        m_pGlyphShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateGlyphRenderPipeline();
    });

    wgpu::BufferDescriptor glyphsBufferDescriptor{
        .label = "Debug render vertex buffer (glyphs)",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(GlyphData)
    };
    m_GlyphVertexBuffer = pDevice.CreateBuffer(&glyphsBufferDescriptor);
    m_GlyphData.reserve(DEBUG_DRAW_VERTEX_BUFFER_SIZE);
}

void DebugRenderImpl::Render(wgpu::RenderPassEncoder& renderPass)
{
    m_LineData.clear();
    m_GlyphData.clear();

    // flush() will populate the vertex data, appending one drawLineList/drawGlyphList batch at
    // a time (each batch is capped at DEBUG_DRAW_VERTEX_BUFFER_SIZE vertices by the library).
    dd::flush();

    if (m_LineRenderPipeline && !m_LineData.empty())
    {
        // dd::flush() can produce far more than one buffer's worth of lines (e.g. a heightfield
        // wireframe), so grow the GPU buffer to fit the whole frame and upload it in one go.
        const uint64_t requiredSize = static_cast<uint64_t>(m_LineData.size()) * sizeof(LineData);
        if (requiredSize > m_LineVertexBufferSize)
        {
            wgpu::BufferDescriptor lineBufferDescriptor{
                .label = "Debug render vertex buffer (lines)",
                .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
                .size = requiredSize
            };
            m_LineVertexBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&lineBufferDescriptor);
            m_LineVertexBufferSize = requiredSize;
        }

        GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_LineVertexBuffer, 0, m_LineData.data(), requiredSize);

        renderPass.SetPipeline(m_LineRenderPipeline);
        renderPass.SetVertexBuffer(0, m_LineVertexBuffer);
        renderPass.Draw(m_LineData.size());
    }

    if (m_GlyphRenderPipeline && !m_GlyphData.empty())
    {
        renderPass.SetBindGroup(1, m_GlyphBindGroup);
        renderPass.SetPipeline(m_GlyphRenderPipeline);
        renderPass.SetVertexBuffer(0, m_GlyphVertexBuffer);
        renderPass.Draw(m_GlyphData.size());
    }
}

dd::GlyphTextureHandle DebugRenderImpl::createGlyphTexture(int width, int height, const void* pPixels)
{
    wgpu::TextureDescriptor textureDesc{
        .label = "Debug render glyph texture",
        .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
        .dimension = wgpu::TextureDimension::e2D,
        .size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
        .format = wgpu::TextureFormat::R8Unorm
    };
    m_GlyphTexture = GetRenderSystem()->GetDevice().CreateTexture(&textureDesc);

    wgpu::TexelCopyTextureInfo destination{
        .texture = m_GlyphTexture,
        .origin = { 0, 0, 0 }
    };

    wgpu::TexelCopyBufferLayout source{
        .bytesPerRow = static_cast<uint32_t>(width),
        .rowsPerImage = static_cast<uint32_t>(height)
    };

    GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, pPixels, width * height, &source, &textureDesc.size);

    wgpu::TextureViewDescriptor textureViewDesc{
        .format = textureDesc.format,
        .dimension = wgpu::TextureViewDimension::e2D,
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
        .aspect = wgpu::TextureAspect::All
    };
    m_GlyphTextureView = m_GlyphTexture.CreateView(&textureViewDesc);

    wgpu::BindGroupLayoutEntry textureLayoutEntry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Fragment,
        .texture{
            .sampleType = wgpu::TextureSampleType::Float,
            .viewDimension = wgpu::TextureViewDimension::e2D }
    };

    wgpu::SamplerDescriptor samplerDesc{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear
    };

    wgpu::Sampler sampler = GetRenderSystem()->GetDevice().CreateSampler(&samplerDesc);

    wgpu::BindGroupLayoutEntry samplerLayoutEntry{
        .binding = 1,
        .visibility = wgpu::ShaderStage::Fragment,
        .sampler{
            .type = wgpu::SamplerBindingType::Filtering }
    };

    wgpu::BindGroupLayoutEntry layoutEntries[] = {
        textureLayoutEntry,
        samplerLayoutEntry
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = 2,
        .entries = layoutEntries
    };
    m_GlyphBindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    wgpu::BindGroupEntry entries[] = {
        { .binding = 0,
            .textureView = m_GlyphTextureView },
        { .binding = 1,
            .sampler = sampler }
    };

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_GlyphBindGroupLayout,
        .entryCount = bindGroupLayoutDescriptor.entryCount,
        .entries = entries
    };

    m_GlyphBindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

    return reinterpret_cast<dd::GlyphTextureHandle>(m_GlyphTexture.Get());
}

void DebugRenderImpl::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex)
{
    m_GlyphTexture.Destroy();
    m_GlyphTexture = nullptr;
}

void DebugRenderImpl::CreateLineRenderPipeline()
{
    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSurfaceFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pUntexturedShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Debug render lines",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pUntexturedShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_C3)
        },
        .primitive = { .topology = wgpu::PrimitiveTopology::LineList },
        .fragment = &fragmentState
    };
    m_LineRenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void DebugRenderImpl::CreateGlyphRenderPipeline()
{
    wgpu::BlendState blendState{
        .color{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrc },
        .alpha{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrc }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSurfaceFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pGlyphShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::BindGroupLayout bindGroupLayouts[] = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_GlyphBindGroupLayout
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 2,
        .bindGroupLayouts = bindGroupLayouts
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Debug render glyphs",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pGlyphShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P2_C3_UV) 
        },
        .fragment = &fragmentState
    };
    m_GlyphRenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void DebugRenderImpl::drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled)
{
}

void DebugRenderImpl::drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled)
{
    // Append this batch; dd::flush() may call us several times per frame. The accumulated data
    // is uploaded and drawn once, back in Render().
    const size_t base = m_LineData.size();
    m_LineData.resize(base + count);
    for (int i = 0; i < count; i++)
    {
        LineData& lineData = m_LineData[base + i];
        const auto& line = lines[i].line;
        lineData.position = glm::vec3(line.x, line.y, line.z);
        lineData.color = glm::vec3(line.r, line.g, line.b);
    }
}

void DebugRenderImpl::drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex)
{
    m_GlyphData.resize(count);
    for (int i = 0; i < count; i++)
    {
        GlyphData& glyphData = m_GlyphData[i];
        const auto& glyph = glyphs[i].glyph;
        glyphData.position = glm::vec2(glyph.x, glyph.y);
        glyphData.color = glm::vec3(glyph.r, glyph.g, glyph.b);
        glyphData.uv = glm::vec2(glyph.u, glyph.v);
    }
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_GlyphVertexBuffer, 0, m_GlyphData.data(), m_GlyphData.size() * sizeof(GlyphData));
}

} // namespace WingsOfSteel::Private