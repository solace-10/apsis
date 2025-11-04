#include "render_pass/threat_indicator_render_pass.hpp"

#include <array>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>
#include <scene/components/transform_component.hpp>
#include <pandora.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render/material.hpp"
#include "render/vertex_types.hpp"
#include "resources/resource.fwd.hpp"
#include "resources/resource_texture_2d.hpp"
#include "systems/threat_indicator_system.hpp"
#include "sector/sector.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

ThreatIndicatorRenderPass::ThreatIndicatorRenderPass()
: RenderPass("Threat indicator render pass")
{
    // Load the debug shader
    GetResourceSystem()->RequestResource("/shaders/vertex_color_texture.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);

        GetResourceSystem()->RequestResource("/ui/icons/chevron.png", [this](ResourceSharedPtr pResource) {
            m_pChevron = std::dynamic_pointer_cast<ResourceTexture2D>(pResource);

            MaterialSpec ms;
            ms.pBaseColorTexture = m_pChevron.get();
            ms.blendMode = BlendMode::Additive;

            m_pChevronMaterial = std::make_unique<Material>(ms);
   
            CreateRenderPipeline();
        });        
    });
}

void ThreatIndicatorRenderPass::CreateRenderPipeline()
{
    if (!m_pShader || !m_pChevronMaterial)
    {
        return;
    }

    wgpu::BlendState blendState = m_pChevronMaterial->GetBlendState();
    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with global uniforms and material bind group
    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_pChevronMaterial->GetBindGroupLayout()
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
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
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_C4_UV)
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

    if (m_RenderPipeline && m_pChevronMaterial && m_VertexBuffer && m_VertexCount > 0)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetBindGroup(1, m_pChevronMaterial->GetBindGroup());
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
    std::vector<VertexP3C4UV> vertices;
    vertices.reserve(threats.size() * 6);

    const float circleRadius = 15.0f;
    const float markerHalfSize = 1.0f;

    float yOffset = 0.0f;
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

        // Define marker vertices as a square (two triangles) pointing along +Z before rotation
        struct VertexData {
            glm::vec3 position;
            glm::vec2 uv;
        };

        VertexData localVertices[6] = {
            // First triangle (bottom-left, bottom-right, top-right)
            { glm::vec3(-markerHalfSize, yOffset, -markerHalfSize), glm::vec2(0.0f, 1.0f) }, // Bottom-left
            { glm::vec3(markerHalfSize, yOffset, -markerHalfSize),  glm::vec2(1.0f, 1.0f) }, // Bottom-right
            { glm::vec3(markerHalfSize, yOffset, markerHalfSize),   glm::vec2(1.0f, 0.0f) }, // Top-right
            // Second triangle (bottom-left, top-right, top-left)
            { glm::vec3(-markerHalfSize, yOffset, -markerHalfSize), glm::vec2(0.0f, 1.0f) }, // Bottom-left
            { glm::vec3(markerHalfSize, yOffset, markerHalfSize),   glm::vec2(1.0f, 0.0f) }, // Top-right
            { glm::vec3(-markerHalfSize, yOffset, markerHalfSize),  glm::vec2(0.0f, 0.0f) }  // Top-left
        };

        // Transform and add vertices
        glm::vec4 color = glm::vec4(0.75f, 0.0f, 0.0f, 0.8f);
        for (int i = 0; i < 6; i++)
        {
            glm::vec3 worldPos = glm::vec3(rotation * glm::vec4(localVertices[i].position, 1.0f)) + markerCenter;
            vertices.push_back({ worldPos, color, localVertices[i].uv });
        }

        yOffset += 0.1f;        
    }

    m_VertexCount = static_cast<uint32_t>(vertices.size());

    // Recreate vertex buffer with new data
    if (m_VertexCount > 0)
    {
        wgpu::Device device = GetRenderSystem()->GetDevice();
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Threat indicator vertex buffer",
            .usage = wgpu::BufferUsage::Vertex,
            .size = vertices.size() * sizeof(VertexP3C4UV),
            .mappedAtCreation = true
        };
        m_VertexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(m_VertexBuffer.GetMappedRange(), vertices.data(), vertices.size() * sizeof(VertexP3C4UV));
        m_VertexBuffer.Unmap();
    }
}

} // namespace WingsOfSteel
