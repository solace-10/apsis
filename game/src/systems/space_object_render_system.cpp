#include <array>
#include <bit>
#include <cmath>
#include <optional>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/color.hpp>
#include <input/input_system.hpp>
#include <pandora.hpp>
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
#include "components/mouse_picking_component.hpp"
#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "components/planet_component.hpp"
#include "components/propagation_failure_component.hpp"
#include "components/propagation_pending_component.hpp"
#include "components/space_object_group_component.hpp"
#include "game.hpp"
#include "sector/group_filter.hpp"
#include "sector/group_filters.hpp"
#include "sector/sector.hpp"
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

// Ensures the labels vertex buffer is reallocated if we would write more data to it than it had been allocated for.
void SpaceObjectRenderSystem::EnsureLabelsVertexBuffer()
{
    const size_t currentSize = m_LabelsVertexData.size() * sizeof(VertexP2C4UV);
    if (currentSize <= m_LabelsVertexBufferSize)
    {
        return;
    }

    m_LabelsVertexBufferSize = currentSize * 2;
    Log::Info() << "Reallocated labels vertex buffer to " << m_LabelsVertexBufferSize << " bytes.";

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Labels vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = m_LabelsVertexBufferSize
    };

    m_LabelsVertexBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);
}

void SpaceObjectRenderSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr || GetActiveScene()->GetCamera() == nullptr || !m_pFont)
    {
        return;
    }

    // First, so that the deselection this can cause is seen by the poll below.
    RetireFailedSpaceObjects();

    // Polled rather than pushed from Sector::SetSelectedSpaceObject(), as the selection can change
    // before any labels exist. Done before the rebuild below so a rebuild sees the current selection.
    UpdateSelectedSpaceObject();

    if (m_LabelsDirty)
    {
        GenerateSpaceObjectGroups();
        GenerateLabelsVertexData();
        m_LabelsDirty = false;
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
    auto view = registry.view<LabelComponent, MousePickingComponent, const TransformComponent>(entt::exclude<PropagationPendingComponent>);
    const CameraComponent& cameraComponent = GetActiveScene()->GetCamera()->GetComponent<CameraComponent>();
    const uint32_t windowWidth = GetWindow()->GetWidth();
    const uint32_t windowHeight = GetWindow()->GetHeight();
    const glm::vec3 cameraPosition = cameraComponent.camera.GetPosition();
    const glm::vec3 cameraForward = glm::normalize(cameraComponent.camera.GetTarget() - cameraPosition);

    // The object under the cursor is resolved as part of this pass, as it already projects every
    // space object into screen space. Doing it here rather than in a pass of its own also means the
    // hovered label is generated from this frame's positions, rather than lagging behind by a frame.
    const std::optional<glm::vec2> cursorPosition = GetInputSystem()->GetCursorPosition();
    const float hoverRadiusSquared = kHoverRadiusPixels * kHoverRadiusPixels;
    EntityHandle hoveredEntityHandle = NullEntityHandle;
    float hoveredDepth = 1.0f;

    view.each([this, &cameraComponent, windowWidth, windowHeight, planetRadiusSquared, &cameraPosition, &cameraForward, &cursorPosition, hoverRadiusSquared, &hoveredEntityHandle, &hoveredDepth](const EntityHandle entityHandle, LabelComponent& labelComponent, MousePickingComponent& mousePickingComponent, const TransformComponent& transformComponent) {
        const glm::vec3 labelPosition = transformComponent.GetTranslation();
        const glm::vec3 d(labelPosition - cameraPosition);

        // Cull objects behind the camera. Without this check, perspective division by
        // negative w mirrors their position and causes erratic screen-space movement.
        if (glm::dot(d, cameraForward) <= 0.0f)
        {
            labelComponent.SetOccluded(true);
            mousePickingComponent.SetEnabled(false);
            return;
        }

        // Check if a line segment between the label and the camera intersects the planet.
        // If so, then this label is occluded.
        bool isOccluded = false;
        const float a = glm::dot(d, d);
        const float b = 2.0f * glm::dot(cameraPosition, d);
        const float c = glm::dot(cameraPosition, cameraPosition) - planetRadiusSquared;
        const float discriminant = b * b - 4.0f * a * c;

        // If the discriminant is < 0.0f, then the line doesn't intersect the planet.
        // We only need to do the more expensive calculations if we need to check the
        // intersection of the line segment.
        if (discriminant >= 0.0f)
        {
            const float sqrtDisc = std::sqrt(discriminant);
            const float t1 = (-b - sqrtDisc) / (2.0f * a);
            const float t2 = (-b + sqrtDisc) / (2.0f * a);
            isOccluded = (t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f);
        }

        labelComponent.SetOccluded(isOccluded);
        mousePickingComponent.SetEnabled(!isOccluded);
        if (!isOccluded)
        {
            // The marker offset is used to centre the marker's character with the satellite's position in screenspace.
            const glm::vec3 markerOffset(8.0f, 10.0f, 0.0f);
            const glm::vec3 screenSpacePosition = cameraComponent.camera.WorldToScreen(labelPosition, windowWidth, windowHeight);
            labelComponent.SetScreenSpacePosition(screenSpacePosition - markerOffset);
            mousePickingComponent.SetScreenSpacePosition(screenSpacePosition);

            if (cursorPosition.has_value())
            {
                const glm::vec2 delta = glm::vec2(screenSpacePosition.x, screenSpacePosition.y) - cursorPosition.value();
                const float distanceSquared = glm::dot(delta, delta);
                if (distanceSquared < hoverRadiusSquared && screenSpacePosition.z <= hoveredDepth)
                {
                    hoveredEntityHandle = entityHandle;
                    hoveredDepth = screenSpacePosition.z;
                }
            }
        }
    });

    UpdateHoveredSpaceObject(hoveredEntityHandle);
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
        // clang-format off
        std::array<wgpu::BindGroupEntry, 2> entries = {
            {
                {
                    .binding = 0,
                    .sampler = m_Sampler
                },
                {
                    .binding = 1,
                    .textureView = m_pFont->GetTexture()->GetTextureView()
                }
            }
        };
        // clang-format on

        wgpu::BindGroupDescriptor bindGroupDesc{
            .layout = m_TextureBindGroupLayout,
            .entryCount = entries.size(),
            .entries = entries.data()
        };
        m_TextureBindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDesc);
    }

    m_LabelsVertexData.clear();

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<LabelComponent>(entt::exclude<PropagationPendingComponent>);

    view.each([this](const auto entity, const LabelComponent& labelComponent) {
        if (labelComponent.IsOccluded())
        {
            return;
        }

        const std::vector<VertexP2C4UV>& vertexData = labelComponent.GetVertexData();
        for (auto vertex : vertexData)
        {
            vertex.position += labelComponent.GetScreenSpacePosition();
            m_LabelsVertexData.push_back(vertex);
        }
    });

    if (m_LabelsVertexData.empty())
    {
        return;
    }

    EnsureLabelsVertexBuffer();
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_LabelsVertexBuffer, 0, m_LabelsVertexData.data(), m_LabelsVertexData.size() * sizeof(VertexP2C4UV));

    renderPass.SetPipeline(m_RenderPipeline);
    renderPass.SetBindGroup(1, m_TextureBindGroup);
    renderPass.SetVertexBuffer(0, m_LabelsVertexBuffer);
    renderPass.Draw(m_LabelsVertexData.size());
}

// Whether every visible object gets a named label, rather than just a location marker.
bool SpaceObjectRenderSystem::ShouldDisplayAllLabels() const
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<MetadataComponent>();

    uint32_t totalVisibleObjects = 0;
    view.each([&totalVisibleObjects](const auto entity, const MetadataComponent& metadataComponent) {
        if (metadataComponent.IsVisible())
        {
            totalVisibleObjects++;
        }
    });
    return (totalVisibleObjects <= 100);
}

// An object gets its name alongside its location marker when all labels are being displayed, or
// when it is the one the cursor is over or the one that is selected.
bool SpaceObjectRenderSystem::ShouldDisplayFullLabel(EntityHandle entityHandle) const
{
    return m_DisplayAllLabels || entityHandle == m_HoveredEntityHandle || entityHandle == m_SelectedEntityHandle;
}

void SpaceObjectRenderSystem::NotifyGroupFiltersChanged()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    registry.clear<LabelComponent, MousePickingComponent, SpaceObjectGroupComponent>();
    m_HoveredEntityHandle = NullEntityHandle;

    Sector* pSector = Game::Get()->GetSector();
    GroupFilters::Mask currentVisibleMask = pSector->GetGroupFilters()->GetCurrentMask();

    EntityHandle currentlySelectedEntityHandle = NullEntityHandle;
    EntitySharedPtr pSelectedSpaceObject = pSector->GetSelectedSpaceObject();
    if (pSelectedSpaceObject)
    {
        currentlySelectedEntityHandle = pSelectedSpaceObject->GetEntityHandle();
    }

    auto view = registry.view<MetadataComponent>();
    view.each([&registry, &currentVisibleMask, currentlySelectedEntityHandle](const EntityHandle entityHandle, MetadataComponent& metadataComponent) {
        const bool isInVisibleGroupFilter = (currentVisibleMask & metadataComponent.GetGroupFilterMask()) != 0;
        const bool isSelected = (currentlySelectedEntityHandle == entityHandle);

        // A retired object stays retired, whatever the filters or the selection say.
        const bool hasFailed = registry.all_of<PropagationFailureComponent>(entityHandle);
        const bool isVisible = !hasFailed && (isInVisibleGroupFilter || isSelected);
        metadataComponent.SetVisible(isVisible);

        if (isVisible)
        {
            if (!registry.all_of<OrbitalStateComponent>(entityHandle))
            {
                registry.emplace<OrbitalStateComponent>(entityHandle);
                registry.emplace<PropagationPendingComponent>(entityHandle);
            }
        }
        else
        {
            registry.remove<OrbitalStateComponent, PropagationPendingComponent>(entityHandle);
        }
    });
    GenerateLabels();
}

void SpaceObjectRenderSystem::GenerateLabels()
{
    m_LabelsDirty = true;
}

void SpaceObjectRenderSystem::GenerateSpaceObjectGroups()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<OrbitalElementsComponent, MetadataComponent>();

    // The group ids handed out below restart from 0 on every call and are used to index
    // m_LabelGroups, so the previous run's groups have to go.
    m_LabelGroups.clear();

    std::unordered_map<size_t, std::vector<entt::entity>> groups;
    view.each([this, &groups](const auto entity, const OrbitalElementsComponent& orbitalElements, const MetadataComponent& metadataComponent) {
        if (!metadataComponent.IsVisible())
        {
            return;
        }

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

    const GroupFilters::Mask currentMask = Game::Get()->GetSector()->GetGroupFilters()->GetCurrentMask();

    // Cached so that regenerating an individual label doesn't have to count the visible objects
    // all over again.
    m_DisplayAllLabels = ShouldDisplayAllLabels();

    view.each([this, currentMask, &registry](const auto entityHandle, const MetadataComponent& metadataComponent) {
        if (!metadataComponent.IsVisible())
        {
            return;
        }

        GenerateLabelVertexData(entityHandle, metadataComponent, currentMask);
        registry.emplace<MousePickingComponent>(entityHandle);
    });
}

// Builds the label for a single space object. Used both by the bulk regeneration and when a single
// label changes, so an object's label is the same either way. A full label is the location marker,
// the object's name and (if used) the size of the group; otherwise it is just the location marker.
void SpaceObjectRenderSystem::GenerateLabelVertexData(EntityHandle entityHandle, const MetadataComponent& metadataComponent, const GroupFilters::Mask& currentMask)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();

    std::stringstream labelStream;

    // We've manually added to the font a "target" square using the usually unprintable code "0x1" (Start Of Heading).
    if (ShouldDisplayFullLabel(entityHandle))
    {
        SpaceObjectGroupComponent* pSpaceObjectGroupComponent = registry.try_get<SpaceObjectGroupComponent>(entityHandle);
        if (pSpaceObjectGroupComponent)
        {
            if (pSpaceObjectGroupComponent->IsPrimaryElement())
            {
                labelStream << "\1" << metadataComponent.GetObjectName() << " (" << m_LabelGroups[pSpaceObjectGroupComponent->GetGroupId()].size() << ")";
            }
        }
        else
        {
            labelStream << "\1" << metadataComponent.GetObjectName();
        }
    }
    else
    {
        labelStream << "\1";
    }

    const std::string label(labelStream.str());
    const bool isCurrentlySelected = (entityHandle == m_SelectedEntityHandle);
    const glm::vec4 labelColor(GetSpaceObjectColor(isCurrentlySelected, currentMask, metadataComponent).AsVec3(), 1.0f);

    // The component is updated in place rather than replaced: its screen space position and
    // occlusion state are maintained by Update() and resetting them here would make the label
    // flicker for a frame when only its text changes.
    LabelComponent& labelComponent = registry.get_or_emplace<LabelComponent>(entityHandle);
    labelComponent.SetText(label);
    labelComponent.SetVertexData(m_pFont->Generate(label, labelColor));
}

// Rebuilds a single object's label from the current state. Tolerates a handle that has no label,
// which happens when the selection outlives the group filter change that cleared the labels.
void SpaceObjectRenderSystem::RegenerateLabel(EntityHandle entityHandle)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    if (!registry.valid(entityHandle) || !registry.all_of<LabelComponent, MetadataComponent>(entityHandle))
    {
        return;
    }

    const GroupFilters::Mask currentMask = Game::Get()->GetSector()->GetGroupFilters()->GetCurrentMask();
    GenerateLabelVertexData(entityHandle, registry.get<MetadataComponent>(entityHandle), currentMask);
}

// Shows the full label for the object under the cursor. Only the labels of the objects entering and
// leaving the hover are regenerated, so this doesn't pay for a rebuild of every object's label.
void SpaceObjectRenderSystem::UpdateHoveredSpaceObject(EntityHandle hoveredEntityHandle)
{
    if (hoveredEntityHandle == m_HoveredEntityHandle)
    {
        return;
    }

    const EntityHandle previouslyHoveredEntityHandle = m_HoveredEntityHandle;
    m_HoveredEntityHandle = hoveredEntityHandle;

    // Both labels are regenerated only after the handle has been updated, so that each one is built
    // from the new state: the object being left may still be the selected one, in which case it
    // keeps its full label rather than dropping back to a marker.
    RegenerateLabel(previouslyHoveredEntityHandle);
    RegenerateLabel(hoveredEntityHandle);
}

// Shows the full label for the selected object, for as long as it stays selected.
void SpaceObjectRenderSystem::UpdateSelectedSpaceObject()
{
    EntityHandle selectedEntityHandle = NullEntityHandle;
    EntitySharedPtr pSelectedSpaceObject = Game::Get()->GetSector()->GetSelectedSpaceObject();
    if (pSelectedSpaceObject)
    {
        selectedEntityHandle = pSelectedSpaceObject->GetEntityHandle();
    }

    if (selectedEntityHandle == m_SelectedEntityHandle)
    {
        return;
    }

    const EntityHandle previouslySelectedEntityHandle = m_SelectedEntityHandle;
    m_SelectedEntityHandle = selectedEntityHandle;

    // As with the hover, the deselected object may still be the hovered one and keep its full label.
    // The labels also carry the selection colour, so both of them change appearance here.
    RegenerateLabel(previouslySelectedEntityHandle);
    RegenerateLabel(selectedEntityHandle);
}

void SpaceObjectRenderSystem::RetireFailedSpaceObjects()
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const PropagationFailureComponent, MetadataComponent>();

    view.each([this, &registry](const EntityHandle entityHandle, const PropagationFailureComponent&, MetadataComponent& metadataComponent) {
        // Already retired: the component is a latch, so there is work here only on the frame an
        // object first fails.
        if (!metadataComponent.IsVisible())
        {
            return;
        }

        metadataComponent.SetVisible(false);
        registry.remove<OrbitalStateComponent, PropagationPendingComponent, LabelComponent, MousePickingComponent, SpaceObjectGroupComponent>(entityHandle);

        if (entityHandle == m_HoveredEntityHandle)
        {
            m_HoveredEntityHandle = NullEntityHandle;
        }

        // Sector::Update() hands the selected object's OrbitalStateComponent to the webapp every
        // frame, and it has just been taken away. m_SelectedEntityHandle is left to the poll.
        if (entityHandle == m_SelectedEntityHandle)
        {
            Game::Get()->GetSector()->SetSelectedSpaceObject(nullptr);
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

// Calculate the color of the space object based on the most important group filter it belongs to.
const Color& SpaceObjectRenderSystem::GetSpaceObjectColor(bool isCurrentlySelected, const GroupFilters::Mask& currentMask, const MetadataComponent& metadataComponent) const
{
    GroupFilters::Mask mask = metadataComponent.GetGroupFilterMask();

    // If the set of groups defined by the UI (`currentMask`) is non-empty, we need to filter this
    // object's group against it.
    // This makes it so that if an object is part of two groups (e.g. `starlink` and `last-30-days`)
    // but the user only has `starlink` selected, then they'll get that group's colour rather than
    // the higher-priority `last-30-days`.
    if (!isCurrentlySelected && currentMask.any())
    {
        mask &= currentMask;
    }

    auto bits = mask.to_ullong();
    int lsb = std::countr_zero(bits);

    GroupFilter* pGroupFilter = Game::Get()->GetSector()->GetGroupFilters()->GetGroupFilter(lsb);
    if (pGroupFilter)
    {
        return pGroupFilter->GetColor();
    }
    else
    {
        static const Color sNoGroupColor(Color::Red);
        return sNoGroupColor;
    }
}

} // namespace WingsOfSteel
