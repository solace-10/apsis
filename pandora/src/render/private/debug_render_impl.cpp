#include <array>

#include "render/private/debug_render_impl.hpp"

#define DEBUG_DRAW_IMPLEMENTATION
#include <debug_draw.hpp>

#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "pandora.hpp"

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

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Debug render vertex buffer (lines)",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(LineData)
    };
    wgpu::Device pDevice = Pandora::GetRenderSystem()->GetDevice();
    m_LineVertexBuffer = pDevice.CreateBuffer(&bufferDescriptor);
    m_LineData.reserve(DEBUG_DRAW_VERTEX_BUFFER_SIZE);
}

void DebugRenderImpl::Render(wgpu::RenderPassEncoder& renderPass)
{
    m_LineData.clear();

    // flush() will populate the vertex data.
    dd::flush();

    if (m_LineRenderPipeline && !m_LineData.empty())
    {
        renderPass.SetPipeline(m_LineRenderPipeline);
        renderPass.SetVertexBuffer(0, m_LineVertexBuffer);
        renderPass.Draw(m_LineData.size());
    }
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
            .shaderLocation = 0
        },
        { // Color
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1
        }
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
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pUntexturedShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::LineList
        },
        .fragment = &fragmentState
    };
    m_LineRenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

// dd::GlyphTextureHandle DebugRenderImpl::createGlyphTexture(int width, int height, const void * pixels)
// {
//     return nullptr;
// }

// void DebugRenderImpl::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex)
// {
// }

void DebugRenderImpl::drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled)
{

}

void DebugRenderImpl::drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled)
{
    m_LineData.resize(count);
    for (int i = 0; i < count; i++)
    {
        LineData& lineData = m_LineData[i];
        lineData.position = glm::vec3(lines[i].line.x, lines[i].line.y, lines[i].line.z);
        lineData.color = glm::vec3(lines[i].line.r, lines[i].line.g, lines[i].line.b);
    }
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_LineVertexBuffer, 0, m_LineData.data(), m_LineData.size() * sizeof(LineData));
}
    
// void DebugRenderImpl::drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex)
// {
// }

} // namespace WingsOfSteel::Pandora::Private