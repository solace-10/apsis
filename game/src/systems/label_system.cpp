#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_shader.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/label_component.hpp"
#include "systems/label_system.hpp"

namespace WingsOfSteel
{

void LabelSystem::Initialize(Scene* pScene)
{
    GetResourceSystem()->RequestResource("/shaders/label.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
    });

    wgpu::Device device = GetRenderSystem()->GetDevice();
    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Label vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = kMaxLabels * kVerticesPerQuad * sizeof(VertexP2C3UV)
    };
    m_VertexBuffer = device.CreateBuffer(&bufferDescriptor);
    m_VertexData.reserve(kMaxLabels * kVerticesPerQuad);
}

void LabelSystem::CreateRenderPipeline()
{
    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Label render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P2_C3_UV) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void LabelSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr || GetActiveScene()->GetCamera() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent, const TransformComponent>();

    const CameraComponent& cameraComponent = GetActiveScene()->GetCamera()->GetComponent<CameraComponent>();
    const uint32_t windowWidth = GetWindow()->GetWidth();
    const uint32_t windowHeight = GetWindow()->GetHeight();
    view.each([&cameraComponent, windowWidth, windowHeight](LabelComponent& labelComponent, const TransformComponent& transformComponent) {
        labelComponent.SetScreenSpacePosition(cameraComponent.camera.WorldToScreen(transformComponent.GetTranslation(), windowWidth, windowHeight));
    });
}

void LabelSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr || !m_RenderPipeline)
    {
        return;
    }

    m_VertexData.clear();

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent>();

    const glm::vec3 quadColor(1.0f, 1.0f, 1.0f); // White

    view.each([this, &quadColor](const auto entity, const LabelComponent& labelComponent) {
        const glm::vec2& pos = labelComponent.GetScreenSpacePosition();

        // Generate 6 vertices for a quad (2 triangles) centered at pos
        // Triangle 1: top-left, top-right, bottom-left
        // Triangle 2: top-right, bottom-right, bottom-left

        float left = pos.x - kQuadHalfSize;
        float right = pos.x + kQuadHalfSize;
        float top = pos.y - kQuadHalfSize;
        float bottom = pos.y + kQuadHalfSize;

        // Triangle 1
        m_VertexData.push_back({ glm::vec2(left, top), quadColor, glm::vec2(0.0f, 0.0f) });
        m_VertexData.push_back({ glm::vec2(right, top), quadColor, glm::vec2(1.0f, 0.0f) });
        m_VertexData.push_back({ glm::vec2(left, bottom), quadColor, glm::vec2(0.0f, 1.0f) });

        // Triangle 2
        m_VertexData.push_back({ glm::vec2(right, top), quadColor, glm::vec2(1.0f, 0.0f) });
        m_VertexData.push_back({ glm::vec2(right, bottom), quadColor, glm::vec2(1.0f, 1.0f) });
        m_VertexData.push_back({ glm::vec2(left, bottom), quadColor, glm::vec2(0.0f, 1.0f) });
    });

    if (m_VertexData.empty())
    {
        return;
    }

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_VertexBuffer, 0, m_VertexData.data(), m_VertexData.size() * sizeof(VertexP2C3UV));

    renderPass.SetPipeline(m_RenderPipeline);
    renderPass.SetVertexBuffer(0, m_VertexBuffer);
    renderPass.Draw(m_VertexData.size());
}

} // namespace WingsOfSteel
