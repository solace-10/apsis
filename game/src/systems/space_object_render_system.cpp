#include <array>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/color.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_bitmap_font.hpp>
#include <resources/resource_shader.hpp>
#include <resources/resource_system.hpp>
#include <resources/resource_texture_2d.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/label_component.hpp"
#include "components/space_object_component.hpp"
#include "components/space_object_group_component.hpp"
#include "render/vertex_types.hpp"
#include "resources/resource.fwd.hpp"
#include "systems/space_object_render_system.hpp"

namespace WingsOfSteel
{

SpaceObjectRenderSystem::SpaceObjectRenderSystem()
{
}

SpaceObjectRenderSystem::~SpaceObjectRenderSystem()
{
}

void SpaceObjectRenderSystem::Initialize(Scene* pScene)
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Create sampler
    wgpu::SamplerDescriptor samplerDesc{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear
    };
    m_Sampler = device.CreateSampler(&samplerDesc);

    // Create texture bind group layout
    std::array<wgpu::BindGroupLayoutEntry, 2> layoutEntries = { { { .binding = 0,
                                                                      .visibility = wgpu::ShaderStage::Fragment,
                                                                      .sampler{ .type = wgpu::SamplerBindingType::Filtering } },
        { .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D } } } };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{
        .entryCount = layoutEntries.size(),
        .entries = layoutEntries.data()
    };
    m_TextureBindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    GetResourceSystem()->RequestResource("/shaders/label.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
    });

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Label vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = kMaxLabels * kVerticesPerQuad * sizeof(VertexP2C4UV)
    };
    m_VertexBuffer = device.CreateBuffer(&bufferDescriptor);
    m_VertexData.reserve(kMaxLabels * kVerticesPerQuad);

    GetResourceSystem()->RequestResource("/bitmap_fonts/SupplyMonoBitmap.fnt", [this](ResourceSharedPtr pResource) {
        m_pFont = std::dynamic_pointer_cast<ResourceBitmapFont>(pResource);
    });
}

void SpaceObjectRenderSystem::CreateRenderPipeline()
{
    wgpu::BlendState blendState{
        .color{
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
        .alpha{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_TextureBindGroupLayout
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Label render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P2_C4_UV) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void SpaceObjectRenderSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr || GetActiveScene()->GetCamera() == nullptr || !m_pFont)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent, const TransformComponent>();

    const CameraComponent& cameraComponent = GetActiveScene()->GetCamera()->GetComponent<CameraComponent>();
    const uint32_t windowWidth = GetWindow()->GetWidth();
    const uint32_t windowHeight = GetWindow()->GetHeight();
    view.each([this, &cameraComponent, windowWidth, windowHeight](LabelComponent& labelComponent, const TransformComponent& transformComponent) {
        labelComponent.SetScreenSpacePosition(cameraComponent.camera.WorldToScreen(transformComponent.GetTranslation(), windowWidth, windowHeight));

        if (labelComponent.GetVertexData().empty())
        {
            // We've manually added to the font a "target" square using the usually unprintable code "0x1" (Start Of Heading).
            const std::string label = "\1" + labelComponent.GetText();
            labelComponent.SetVertexData(m_pFont->Generate(label));
        }
    });
}

void SpaceObjectRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr || !m_RenderPipeline || !m_pFont || !m_pFont->GetTexture())
    {
        return;
    }

    // Create texture bind group lazily once the font texture is available
    if (!m_TextureBindGroup)
    {
        std::array<wgpu::BindGroupEntry, 2> entries = { { { .binding = 0,
                                                              .sampler = m_Sampler },
            { .binding = 1,
                .textureView = m_pFont->GetTexture()->GetTextureView() } } };

        wgpu::BindGroupDescriptor bindGroupDesc{
            .layout = m_TextureBindGroupLayout,
            .entryCount = entries.size(),
            .entries = entries.data()
        };
        m_TextureBindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDesc);
    }

    m_VertexData.clear();

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent>();

    view.each([this](const auto entity, const LabelComponent& labelComponent) {
        const std::vector<VertexP2C4UV>& vertexData = labelComponent.GetVertexData();
        for (auto vertex : vertexData)
        {
            vertex.position += labelComponent.GetScreenSpacePosition();
            // vertex.position += glm::floor(labelComponent.GetScreenSpacePosition());
            m_VertexData.push_back(vertex);
        }
    });

    if (m_VertexData.empty())
    {
        return;
    }

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_VertexBuffer, 0, m_VertexData.data(), m_VertexData.size() * sizeof(VertexP2C4UV));

    renderPass.SetPipeline(m_RenderPipeline);
    renderPass.SetBindGroup(1, m_TextureBindGroup);
    renderPass.SetVertexBuffer(0, m_VertexBuffer);
    renderPass.Draw(m_VertexData.size());
}

void SpaceObjectRenderSystem::GenerateSpaceObjectGroups()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<SpaceObjectComponent>();

    std::unordered_map<size_t, std::vector<entt::entity>> groups;
    view.each([this, &groups](const auto entity, const SpaceObjectComponent& component) {
        size_t key = MakeOrbitalKey(component.GetSpaceObject());
        groups[key].push_back(entity);
    });

    SpaceObjectGroupId groupId = 0;
    for (auto& group : groups)
    {
        if (group.second.size() > 1)
        {
            for (const auto& entityHandle : group.second)
            {
                registry.emplace<SpaceObjectGroupComponent>(entityHandle, groupId);
            }
            Log::Info() << "Generated space object group " << groupId << " with " << group.second.size() << " objects.";
            groupId++;
        }
    }
}

void SpaceObjectRenderSystem::GenerateLabels()
{
}

/*
MakeOrbitalKey generates a hash from a SpaceObject's orbital parameters.
To be at the same position, objects need matching orbital elements:
- Inclination, RAAN, Argument of Pericenter, Mean Motion: define the orbital plane and shape
- Eccentricity: defines the orbital shape
- Mean Anomaly: defines position along the orbit

Note: We're comparing at face value without epoch propagation, so this works best
for objects with the same epoch (like docked spacecraft sharing TLE data).
*/
size_t SpaceObjectRenderSystem::MakeOrbitalKey(const SpaceObject& object) const
{
    // Quantize orbital elements:
    // Angles: 0.01 degree precision (2 decimal places)
    // Mean motion / eccentricity: 0.0001 precision (4 decimal places)
    auto quantize2 = [](float v) { return static_cast<int32_t>(std::round(v * 100.0f)); };
    auto quantize4 = [](float v) { return static_cast<int32_t>(std::round(v * 10000.0f)); };

    const int32_t inc = quantize2(object.GetInclination());
    const int32_t raan = quantize2(object.GetRightAscensionOfAscendingNode());
    const int32_t aop = quantize2(object.GetArgumentOfPericenter());
    const int32_t ma = quantize2(object.GetMeanAnomaly());
    const int32_t mm = quantize4(object.GetMeanMotion());
    const int32_t ecc = quantize4(object.GetEccentricity());

    // Combine hashes using boost-style hash combining.
    size_t hash = 0;
    auto hashCombine = [&hash](int32_t v) {
        hash ^= std::hash<int32_t>{}(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    };
    hashCombine(inc);
    hashCombine(raan);
    hashCombine(aop);
    hashCombine(ma);
    hashCombine(mm);
    hashCombine(ecc);

    return hash;
}

} // namespace WingsOfSteel
