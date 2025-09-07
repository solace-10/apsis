#include <array>

#include "render/private/debug_render_impl.hpp"

#define DEBUG_DRAW_IMPLEMENTATION
#include <debug_draw.hpp>

#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"

namespace WingsOfSteel::Pandora::Private
{

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

    Pandora::GetResourceSystem()->RequestResource("/shaders/debug_render_untextured.wgsl", [this](Pandora::ResourceSharedPtr pResource) {
        m_pUntexturedShader = std::dynamic_pointer_cast<Pandora::ResourceShader>(pResource);
        CreateLineRenderPipeline();
    });

    wgpu::Device pDevice = Pandora::GetRenderSystem()->GetDevice();
    wgpu::BufferDescriptor linesBufferDescriptor{
        .label = "Debug render vertex buffer (lines)",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(LineData)
    };
    m_LineVertexBuffer = pDevice.CreateBuffer(&linesBufferDescriptor);
    m_LineData.reserve(DEBUG_DRAW_VERTEX_BUFFER_SIZE);

    Pandora::GetResourceSystem()->RequestResource("/shaders/debug_render_glyph.wgsl", [this](Pandora::ResourceSharedPtr pResource) {
        m_pGlyphShader = std::dynamic_pointer_cast<Pandora::ResourceShader>(pResource);
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

    // flush() will populate the vertex data.
    dd::flush();

    if (m_LineRenderPipeline && !m_LineData.empty())
    {
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

    wgpu::ImageCopyTexture destination{
        .texture = m_GlyphTexture,
        .origin = { 0, 0, 0 }
    };

    wgpu::TextureDataLayout source{
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
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pUntexturedShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    static const int sAttributeCount = 2;
    wgpu::VertexAttribute attributes[sAttributeCount] = {
        { // Position
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0 },
        { // Color
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1 }
    };

    wgpu::VertexBufferLayout vertexBufferLayout{
        .arrayStride = 6 * sizeof(float),
        .stepMode = wgpu::VertexStepMode::Vertex,
        .attributeCount = sAttributeCount,
        .attributes = attributes
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
            .buffers = &vertexBufferLayout },
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
        .format = GetWindow()->GetTextureFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pGlyphShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    static const int sAttributeCount = 3;
    wgpu::VertexAttribute attributes[sAttributeCount] = {
        { // Position
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0 },
        { // Color
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1 },
        { // UV
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 5 * sizeof(float),
            .shaderLocation = 2 }
    };

    wgpu::VertexBufferLayout vertexBufferLayout{
        .arrayStride = 7 * sizeof(float),
        .stepMode = wgpu::VertexStepMode::Vertex,
        .attributeCount = sAttributeCount,
        .attributes = attributes
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
            .buffers = &vertexBufferLayout },
        .fragment = &fragmentState
    };
    m_GlyphRenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void DebugRenderImpl::drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled)
{
}

void DebugRenderImpl::drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled)
{
    m_LineData.resize(count);
    for (int i = 0; i < count; i++)
    {
        LineData& lineData = m_LineData[i];
        const auto& line = lines[i].line;
        lineData.position = glm::vec3(line.x, line.y, line.z);
        lineData.color = glm::vec3(line.r, line.g, line.b);
    }
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_LineVertexBuffer, 0, m_LineData.data(), m_LineData.size() * sizeof(LineData));
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

} // namespace WingsOfSteel::Pandora::Private