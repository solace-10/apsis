#include "render_pass/threat_indicator_render_pass.hpp"

#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>
#include <pandora.hpp>

namespace WingsOfSteel
{

ThreatIndicatorRenderPass::ThreatIndicatorRenderPass()
: RenderPass("Threat indicator render pass")
{
    // Load the debug shader
    GetResourceSystem()->RequestResource("/shaders/debug_render_untextured.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
    });

    // Create vertex data for a square (2 triangles)
    // Square centered at (0,0,0) with size 10x10
    std::vector<VertexP3C3> vertices = {
        // Triangle 1
        { glm::vec3(-5.0f, -5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }, // Bottom-left, red
        { glm::vec3(5.0f, -5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },  // Bottom-right, red
        { glm::vec3(5.0f, 5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },   // Top-right, red
        // Triangle 2
        { glm::vec3(-5.0f, -5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }, // Bottom-left, red
        { glm::vec3(5.0f, 5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },   // Top-right, red
        { glm::vec3(-5.0f, 5.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }   // Top-left, red
    };

    // Create vertex buffer
    wgpu::Device device = GetRenderSystem()->GetDevice();
    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Threat indicator vertex buffer",
        .usage = wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(VertexP3C3),
        .mappedAtCreation = true
    };
    m_VertexBuffer = device.CreateBuffer(&bufferDescriptor);
    memcpy(m_VertexBuffer.GetMappedRange(), vertices.data(), vertices.size() * sizeof(VertexP3C3));
    m_VertexBuffer.Unmap();

    m_Initialized = true;
}

void ThreatIndicatorRenderPass::CreateRenderPipeline()
{
    if (!m_pShader || !m_Initialized)
    {
        return;
    }

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with global uniforms
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Depth state
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less
    };

    // Create the render pipeline
    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Threat indicator render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_C3)
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .cullMode = wgpu::CullMode::None
        },
        .depthStencil = &depthState,
        .multisample = {
            .count = RenderSystem::MsaaSampleCount
        },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void ThreatIndicatorRenderPass::Render(wgpu::CommandEncoder& encoder)
{
    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = GetWindow()->GetMsaaColorTexture().GetTextureView(),
        .resolveTarget = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Load,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }
    };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{
        .view = GetWindow()->GetDepthTexture().GetTextureView(),
        .depthLoadOp = wgpu::LoadOp::Load,
        .depthStoreOp = wgpu::StoreOp::Store,
        .depthClearValue = 1.0f
    };

    wgpu::RenderPassDescriptor renderpass{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderpass);
    GetRenderSystem()->UpdateGlobalUniforms(renderPass);

    // Draw the square if the pipeline is ready
    if (m_RenderPipeline && m_VertexBuffer)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetVertexBuffer(0, m_VertexBuffer);
        renderPass.Draw(6); // 6 vertices (2 triangles)
    }

    renderPass.End();
}

} // namespace WingsOfSteel