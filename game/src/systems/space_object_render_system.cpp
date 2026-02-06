#include <array>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/color.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <render/rendersystem.hpp>
#include <render/vertex_types.hpp>
#include <render/window.hpp>
#include <resources/resource_bitmap_font.hpp>
#include <resources/resource_shader.hpp>
#include <resources/resource_system.hpp>
#include <resources/resource_texture_2d.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

#include "components/label_component.hpp"
#include "components/metadata_component.hpp"
#include "components/orbital_elements_component.hpp"
#include "components/planet_component.hpp"
#include "components/space_object_group_component.hpp"
#include "sector/sector.hpp"
#include "systems/space_object_render_system.hpp"
#include "game.hpp"

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

    // clang-format off
    // Create texture bind group layout
    std::array<wgpu::BindGroupLayoutEntry, 2> layoutEntries = {{
        {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler{ .type = wgpu::SamplerBindingType::Filtering }
        },
        {
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D
            }
        }
    }};
    // clang-format on

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

    EntitySharedPtr pEarth = Game::Get()->GetSector()->GetEarth();
    if (!pEarth || !pEarth->HasComponent<PlanetComponent>())
    {
        return;
    }

    // To simplify label occlusion calculations, we assume Earth is a sphere and
    // just use the semi-major radius.
    const float planetRadius = pEarth->GetComponent<PlanetComponent>().semiMajorRadius;
    const float planetRadiusSquared = planetRadius * planetRadius;

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent, const TransformComponent>();
    const CameraComponent& cameraComponent = GetActiveScene()->GetCamera()->GetComponent<CameraComponent>();
    const uint32_t windowWidth = GetWindow()->GetWidth();
    const uint32_t windowHeight = GetWindow()->GetHeight();
    view.each([this, &cameraComponent, windowWidth, windowHeight, planetRadiusSquared](LabelComponent& labelComponent, const TransformComponent& transformComponent) {

        const glm::vec3 cameraPosition = cameraComponent.camera.GetPosition();
        const glm::vec3 labelPosition = transformComponent.GetTranslation();

        // Check if a line segment between the label and the camera intersects the planet.
        // If so, then this label is occluded.
        bool isOccluded = false;
        const glm::vec3 d(labelPosition - cameraPosition);
        const float a = glm::dot(d, d);
        const float b = 2.0f * glm::dot(labelPosition, d);
        const float c = glm::dot(labelPosition, labelPosition) - planetRadiusSquared;
        const float discriminant = b * b - 4.0f * a * c;

        // If the discriminant is < 0.0f, then the line doesn't intersect the planet.
        // We only need to do the more expensive calculations if we need to check the
        // intersection of the line segment.
        if (discriminant >= 0.0f)
        {
            const float sqrtDisc = std::sqrt(discriminant);
            const float t1 = (-b - sqrtDisc) / (2.0f * a);
            const float t2 = (-b + sqrtDisc) / (2.0f * a);
            isOccluded = (t1 < 0.0f || t1 > 1.0f) && (t2 < 0.0f || t2 > 1.0f);
        }

        labelComponent.SetOccluded(isOccluded);
        if (!isOccluded)
        {
            labelComponent.SetScreenSpacePosition(cameraComponent.camera.WorldToScreen(labelPosition, windowWidth, windowHeight));
        }
    });
}

void SpaceObjectRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr || !m_RenderPipeline || !m_pFont || !m_pFont->GetTexture())
    {
        return;
    }

    if (m_LabelsDirty)
    {
        GenerateSpaceObjectGroups();
        GenerateLabelsVertexData();
        m_LabelsDirty = false;
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
        if (labelComponent.IsOccluded())
        {
            return;
        }

        const std::vector<VertexP2C4UV>& vertexData = labelComponent.GetVertexData();
        for (auto vertex : vertexData)
        {
            vertex.position += labelComponent.GetScreenSpacePosition();
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

void SpaceObjectRenderSystem::GenerateLabels()
{
    m_LabelsDirty = true;
}

void SpaceObjectRenderSystem::GenerateSpaceObjectGroups()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<OrbitalElementsComponent, MetadataComponent>();

    std::unordered_map<size_t, std::vector<entt::entity>> groups;
    view.each([this, &groups](const auto entity, const OrbitalElementsComponent& orbitalElements, const MetadataComponent& metadata) {
        size_t key = MakeOrbitalKey(orbitalElements);
        groups[key].push_back(entity);
    });

    SpaceObjectGroupId groupId = 0;
    for (auto& group : groups)
    {
        if (group.second.size() == 1)
        {
            continue;
        }

        // Create a new, empty group.
        // As the groupId starts at 0 and increments monotonically, it can be used as the index for m_LabelGroups.
        m_LabelGroups.push_back(std::vector<entt::entity>());

        Log::Info() << "Generating space object group " << groupId << " with " << group.second.size() << " objects.";
        for (const auto& entityHandle : group.second)
        {
            const bool isImportant = registry.get<MetadataComponent>(entityHandle).m_IsImportant;
            registry.emplace<SpaceObjectGroupComponent>(entityHandle, groupId, isImportant);
            m_LabelGroups[groupId].push_back(entityHandle);
        }
        groupId++;
    }
}

void SpaceObjectRenderSystem::GenerateLabelsVertexData()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<MetadataComponent>();

    view.each([this, &registry](const auto entityHandle, const MetadataComponent& metadata) {
        std::stringstream labelStream;

        // We've manually added to the font a "target" square using the usually unprintable code "0x1" (Start Of Heading).
        SpaceObjectGroupComponent* pSpaceObjectGroupComponent = registry.try_get<SpaceObjectGroupComponent>(entityHandle);
        if (pSpaceObjectGroupComponent)
        {
            if (pSpaceObjectGroupComponent->IsPrimaryElement())
            {
                labelStream << "\1" << metadata.m_ObjectName << " (" << m_LabelGroups[pSpaceObjectGroupComponent->GetGroupId()].size() << ")";
            }
        }
        else
        {
            labelStream << "\1" << metadata.m_ObjectName;
        }

        const std::string label(labelStream.str());
        if (!label.empty())
        {
            LabelComponent& labelComponent = registry.emplace<LabelComponent>(entityHandle, label);
            labelComponent.SetVertexData(m_pFont->Generate(label));
        }
    });
}

/*
MakeOrbitalKey generates a hash from orbital parameters.
To be at the same position, objects need matching orbital elements:
- Inclination, RAAN, Argument of Pericenter, Mean Motion: define the orbital plane and shape
- Eccentricity: defines the orbital shape
- Mean Anomaly: defines position along the orbit

Note: We're comparing at face value without epoch propagation, so this works best
for objects with the same epoch (like docked spacecraft sharing TLE data).
*/
size_t SpaceObjectRenderSystem::MakeOrbitalKey(const OrbitalElementsComponent& orbitalElements) const
{
    // Quantize orbital elements:
    // Angles: 0.01 degree precision (2 decimal places)
    // Mean motion / eccentricity: 0.0001 precision (4 decimal places)
    auto quantize2 = [](float v) { return static_cast<int32_t>(std::round(v * 100.0f)); };
    auto quantize4 = [](float v) { return static_cast<int32_t>(std::round(v * 10000.0f)); };

    const int32_t inc = quantize2(orbitalElements.GetInclination());
    const int32_t raan = quantize2(orbitalElements.GetRightAscensionOfAscendingNode());
    const int32_t aop = quantize2(orbitalElements.GetArgumentOfPericenter());
    const int32_t ma = quantize2(orbitalElements.GetMeanAnomaly());
    const int32_t mm = quantize4(orbitalElements.GetMeanMotion());
    const int32_t ecc = quantize4(orbitalElements.GetEccentricity());

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
