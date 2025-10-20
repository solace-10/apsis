#include "render_pass/threat_indicator_render_pass.hpp"

#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>
#include <scene/components/transform_component.hpp>
#include <pandora.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "systems/threat_indicator_system.hpp"
#include "sector/sector.hpp"
#include "game.hpp"

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

    UpdateVertexBuffer();

    // Draw the threat markers if the pipeline is ready and we have vertices
    if (m_RenderPipeline && m_VertexBuffer && m_VertexCount > 0)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetVertexBuffer(0, m_VertexBuffer);
        renderPass.Draw(m_VertexCount);
    }

    renderPass.End();
}

void ThreatIndicatorRenderPass::UpdateVertexBuffer()
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    ThreatIndicatorSystem* pThreatIndicatorSystem = pSector->GetSystem<ThreatIndicatorSystem>();
    if (!pThreatIndicatorSystem)
    {
        return;
    }

    EntitySharedPtr pPlayerMech = pSector->GetPlayerMech();
    if (!pPlayerMech || !pPlayerMech->HasComponent<TransformComponent>())
    {
        return;
    }

    const TransformComponent& transformComponent = pPlayerMech->GetComponent<TransformComponent>();
    const glm::vec3 playerPosition = transformComponent.transform[3];
    const std::vector<Threat>& threats = pThreatIndicatorSystem->GetThreats();

    if (threats.empty())
    {
        m_VertexCount = 0;
        return;
    }

    // Collect all vertices for all threat markers
    std::vector<VertexP3C3> vertices;
    vertices.reserve(threats.size() * 6); // 6 vertices per marker (2 triangles)

    const float circleRadius = 10.0f; // 20 units diameter = 10 units radius
    const float markerHalfSize = 2.0f; // Marker is 4x4 units

    for (const Threat& threat : threats)
    {
        // Calculate direction from player to threat on XZ plane
        glm::vec3 direction = threat.position - playerPosition;
        direction.y = 0.0f; // Project onto XZ plane

        if (glm::length(direction) < 0.001f)
        {
            continue; // Skip threats at same position as player
        }

        direction = glm::normalize(direction);

        // Position marker on circle around player
        glm::vec3 markerCenter = playerPosition + direction * circleRadius;
        markerCenter.y = playerPosition.y; // Keep at player's Y level

        // Calculate rotation angle to point towards threat
        // Default marker points along +Z, so we calculate angle from +Z axis
        float angle = atan2(direction.x, direction.z);

        // Create rotation matrix
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Define marker vertices (arrow/triangle shape pointing along +Z before rotation)
        glm::vec3 localVertices[6] = {
            // Triangle pointing towards threat (along +Z)
            glm::vec3(-markerHalfSize, 0.0f, -markerHalfSize), // Bottom-left
            glm::vec3(markerHalfSize, 0.0f, -markerHalfSize),  // Bottom-right
            glm::vec3(0.0f, 0.0f, markerHalfSize),             // Top (point)
            // Back triangle (base)
            glm::vec3(-markerHalfSize, 0.0f, -markerHalfSize), // Bottom-left
            glm::vec3(0.0f, 0.0f, markerHalfSize),             // Top
            glm::vec3(markerHalfSize, 0.0f, -markerHalfSize)   // Bottom-right
        };

        // Transform and add vertices
        glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f); // Red
        for (int i = 0; i < 6; i++)
        {
            glm::vec3 worldPos = glm::vec3(rotation * glm::vec4(localVertices[i], 1.0f)) + markerCenter;
            vertices.push_back({ worldPos, color });
        }
    }

    m_VertexCount = static_cast<uint32_t>(vertices.size());

    // Recreate vertex buffer with new data
    if (m_VertexCount > 0)
    {
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
    }
}

} // namespace WingsOfSteel
