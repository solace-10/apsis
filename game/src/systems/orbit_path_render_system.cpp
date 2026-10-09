#include <cmath>
#include <span>

#include <glm/glm.hpp>

#include <core/color.hpp>
#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>

#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "components/propagation_failure_component.hpp"
#include "components/propagation_pending_component.hpp"
#include "components/sgp4_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/orbit_path_render_system.hpp"
#include "systems/space_object_render_system.hpp"

namespace WingsOfSteel
{

namespace
{

    constexpr float kRibbonHalfWidthPixels = 1.5f;

    // Against the whole revolution rather than a fixed distance, so LEO and GEO look the same.
    constexpr float kDashesPerOrbit = 96.0f;

    constexpr float kSelectedAlpha = 0.9f;
    constexpr float kHoveredAlpha = 0.45f;

    // What is left of a path where the planet is in front of it.
    constexpr float kOccludedAlphaScale = 0.2f;

    // WebGPU's minUniformBufferOffsetAlignment, which a bind group entry's offset must divide.
    constexpr uint64_t kUniformSlotStride = 256;

} // namespace

OrbitPathRenderSystem::OrbitPathRenderSystem()
{
}

OrbitPathRenderSystem::~OrbitPathRenderSystem()
{
    // The pass list outlives the scene, so a pass registered here has to be withdrawn here.
    RenderSystem* pRenderSystem = GetRenderSystem();
    if (pRenderSystem && m_pComputePass)
    {
        pRenderSystem->RemovePass(m_pComputePass);
    }

    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void OrbitPathRenderSystem::Initialize(Scene* pScene)
{
    CreateBindGroupLayout();
    CreateBuffers();

    GetResourceSystem()->RequestResource("/shaders/orbit_path.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipelines();
        HandleShaderInjection();
    });

    m_pComputePass = std::make_shared<SGP4ComputePass>();
    GetRenderSystem()->AddPass(m_pComputePass);
}

void OrbitPathRenderSystem::CreateBindGroupLayout()
{
    // Declared rather than reflected out of the pipeline, so the bind groups can be built before
    // the shader has loaded.
    // clang-format off
    std::array<wgpu::BindGroupLayoutEntry, 2> layoutEntries = {{
        {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Vertex,
            .buffer{ .type = wgpu::BufferBindingType::ReadOnlyStorage }
        },
        {
            .binding = 1,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer{ .type = wgpu::BufferBindingType::Uniform, .minBindingSize = sizeof(OrbitPathUniforms) }
        }
    }};
    // clang-format on

    wgpu::BindGroupLayoutDescriptor descriptor{
        .label = "Orbit path bind group layout",
        .entryCount = static_cast<uint32_t>(layoutEntries.size()),
        .entries = layoutEntries.data()
    };
    m_BindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&descriptor);
}

void OrbitPathRenderSystem::CreateBuffers()
{
    wgpu::Device& device = GetRenderSystem()->GetDevice();

    // A path is always propagated at exactly kOrbitPathSampleCount points and truncation only ever
    // draws fewer, so this never needs to grow.
    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        wgpu::BufferDescriptor pointsDescriptor{
            .label = "Orbit path points buffer",
            .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
            .size = kOrbitPathSampleCount * sizeof(OrbitPathPointGPU)
        };
        m_PointsBuffers[lane] = device.CreateBuffer(&pointsDescriptor);
    }

    wgpu::BufferDescriptor uniformsDescriptor{
        .label = "Orbit path uniforms buffer",
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
        .size = kUniformSlotStride * kLaneCount * kPassCount
    };
    m_UniformsBuffer = device.CreateBuffer(&uniformsDescriptor);

    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        for (size_t pass = 0; pass < kPassCount; pass++)
        {
            // clang-format off
            std::array<wgpu::BindGroupEntry, 2> entries = {{
                {
                    .binding = 0,
                    .buffer = m_PointsBuffers[lane],
                    .size = kOrbitPathSampleCount * sizeof(OrbitPathPointGPU)
                },
                {
                    .binding = 1,
                    .buffer = m_UniformsBuffer,
                    .offset = UniformSlot(lane, pass) * kUniformSlotStride,
                    .size = sizeof(OrbitPathUniforms)
                }
            }};
            // clang-format on

            wgpu::BindGroupDescriptor descriptor{
                .label = "Orbit path bind group",
                .layout = m_BindGroupLayout,
                .entryCount = static_cast<uint32_t>(entries.size()),
                .entries = entries.data()
            };
            m_BindGroups[UniformSlot(lane, pass)] = device.CreateBindGroup(&descriptor);
        }
    }
}

void OrbitPathRenderSystem::CreateRenderPipelines()
{
    m_VisiblePipeline = CreateRenderPipeline("Orbit path render pipeline", wgpu::CompareFunction::Less);
    m_OccludedPipeline = CreateRenderPipeline("Orbit path occluded render pipeline", wgpu::CompareFunction::Greater);
}

wgpu::RenderPipeline OrbitPathRenderSystem::CreateRenderPipeline(const char* pLabel, wgpu::CompareFunction depthCompare)
{
    if (!m_pShader)
    {
        return nullptr;
    }

    // Premultiplied, which is what orbit_path.wgsl returns.
    wgpu::BlendState blendState{
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSurfaceFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .entryPoint = "fragmentMain",
        .targetCount = 1,
        .targets = &colorTargetState
    };

    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_BindGroupLayout
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Never written: the ribbon overlaps itself where the near and far halves of the orbit cross on
    // screen, and the first drawn would cut a hole in the second.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = depthCompare
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = pLabel,
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .entryPoint = "vertexMain",
            .bufferCount = 0,
            .buffers = nullptr },
        // No culling: a segment's winding depends on where the camera has orbited to.
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleStrip, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };

    return GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void OrbitPathRenderSystem::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (m_pShader.get() == pResourceShader)
                {
                    CreateRenderPipelines();
                }
            });
    }
}

void OrbitPathRenderSystem::Update(float delta)
{
    Scene* pScene = GetActiveScene();
    if (pScene == nullptr)
    {
        return;
    }

    entt::registry& registry = pScene->GetRegistry();

    ResolveTrackedObjects(registry);

    if (!m_pComputePass->IsReady())
    {
        return;
    }

    // One instant for the whole frame, or the two lanes describe different moments.
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    UpdateRoster(registry);
    UpdateTimes(registry, now);
    ApplyPropagatedPaths();
    UpdateUniforms(registry);
}

// The same exclusions the marker is subject to: an object that is not on screen should not have an
// orbit on screen either.
bool OrbitPathRenderSystem::CanDrawPath(entt::registry& registry, EntityHandle entityHandle) const
{
    if (entityHandle == NullEntityHandle || !registry.valid(entityHandle))
    {
        return false;
    }

    // OrbitalStateComponent is what marks an object as one the user can currently see.
    if (!registry.all_of<SGP4Component, OrbitalElementsComponent, OrbitalStateComponent>(entityHandle))
    {
        return false;
    }

    // Failed objects lose their OrbitalStateComponent too, in RetireFailedSpaceObjects(), but only
    // on the frame after the failure lands.
    if (registry.any_of<PropagationPendingComponent, PropagationFailureComponent>(entityHandle))
    {
        return false;
    }

    return OrbitalPeriodMinutes(registry.get<OrbitalElementsComponent>(entityHandle).GetMeanMotion()) > 0.0;
}

// Polled rather than pushed from Sector::SetSelectedSpaceObject(), as the selection can change
// before this system exists - the same reason SpaceObjectRenderSystem polls it.
void OrbitPathRenderSystem::ResolveTrackedObjects(entt::registry& registry)
{
    EntityHandle selectedEntityHandle = NullEntityHandle;
    if (EntitySharedPtr pSelectedSpaceObject = Game::Get()->GetSector()->GetSelectedSpaceObject())
    {
        selectedEntityHandle = pSelectedSpaceObject->GetEntityHandle();
    }

    EntityHandle hoveredEntityHandle = NullEntityHandle;
    if (SpaceObjectRenderSystem* pSpaceObjectRenderSystem = GetActiveScene()->GetSystem<SpaceObjectRenderSystem>())
    {
        hoveredEntityHandle = pSpaceObjectRenderSystem->GetHoveredSpaceObject();
    }

    m_Lanes[kSelectedLane] = CanDrawPath(registry, selectedEntityHandle) ? selectedEntityHandle : NullEntityHandle;
    m_Lanes[kHoveredLane] = CanDrawPath(registry, hoveredEntityHandle) ? hoveredEntityHandle : NullEntityHandle;
}

// Uploads the coefficients, and only when the pair of objects being propagated has changed. They
// are fixed for the life of an object.
void OrbitPathRenderSystem::UpdateRoster(entt::registry& registry)
{
    // A borrowed lane is dispatched but never drawn - see IsLaneDrawable(). Keeping the count fixed
    // is what stops SGP4ComputePass rebuilding its buffers every time the cursor crosses an object.
    std::array<EntityHandle, kLaneCount> dispatchLanes{
        m_Lanes[kSelectedLane] != NullEntityHandle ? m_Lanes[kSelectedLane] : m_Lanes[kHoveredLane],
        m_Lanes[kHoveredLane] != NullEntityHandle ? m_Lanes[kHoveredLane] : m_Lanes[kSelectedLane]
    };

    if (dispatchLanes == m_DispatchLanes)
    {
        return;
    }
    m_DispatchLanes = dispatchLanes;

    if (dispatchLanes[kSelectedLane] == NullEntityHandle)
    {
        // An empty roster releases the pass's buffers and stops it dispatching.
        m_pRoster = std::make_shared<const EntityRoster>();
        m_OrbitalElements.clear();
        m_pComputePass->SetOrbitalElements(m_OrbitalElements, m_pRoster);
        return;
    }

    m_OrbitalElements.clear();
    m_OrbitalElements.reserve(kLaneCount * kOrbitPathSampleCount);
    m_RosterScratch.clear();
    m_RosterScratch.reserve(kLaneCount * kOrbitPathSampleCount);

    for (const EntityHandle entityHandle : dispatchLanes)
    {
        const SGP4StepInput input = MakeSGP4StepInput(registry.get<SGP4Component>(entityHandle).m_ElementSet);
        for (size_t i = 0; i < kOrbitPathSampleCount; i++)
        {
            m_OrbitalElements.push_back(input);
            m_RosterScratch.push_back(entityHandle);
        }
    }

    // A new roster rather than a mutated one, so a readback still in flight keeps the one it was
    // dispatched with.
    m_pRoster = std::make_shared<const EntityRoster>(m_RosterScratch);
    m_pComputePass->SetOrbitalElements(m_OrbitalElements, m_pRoster);
}

// Slides both windows forward, every frame: this is what keeps the object on the boundary between
// the solid half and the dashed one as time passes.
void OrbitPathRenderSystem::UpdateTimes(entt::registry& registry, const std::chrono::system_clock::time_point& instant)
{
    if (!m_pRoster || m_pRoster->empty())
    {
        return;
    }

    m_Times.clear();
    m_Times.reserve(m_pRoster->size());

    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        const EntityHandle entityHandle = (*m_pRoster)[lane * kOrbitPathSampleCount];
        const OrbitalElementsComponent* pOrbitalElements = registry.valid(entityHandle) ? registry.try_get<OrbitalElementsComponent>(entityHandle) : nullptr;

        // The count has to match the upload exactly or the two lanes would be propagated to each
        // other's times, so a destroyed object's lane is zero-filled rather than dropped.
        if (!pOrbitalElements)
        {
            m_Times.insert(m_Times.end(), kOrbitPathSampleCount, 0.0f);
            continue;
        }

        const double tsinceMinutes = std::chrono::duration<double, std::ratio<60>>(instant - pOrbitalElements->GetEpoch()).count();
        const double periodMinutes = OrbitalPeriodMinutes(pOrbitalElements->GetMeanMotion());
        const std::vector<float> laneTimes = BuildOrbitPathSampleTimes(tsinceMinutes, periodMinutes, kOrbitPathSampleCount);
        m_Times.insert(m_Times.end(), laneTimes.begin(), laneTimes.end());
    }

    m_pComputePass->SetTimes(m_Times, instant);
}

// Turns the last completed readback into two polylines, once, when it lands.
void OrbitPathRenderSystem::ApplyPropagatedPaths()
{
    const PropagationResults& results = m_pComputePass->GetResults();
    if (!results.pEntities || results.time == m_LastAppliedResultsTime)
    {
        return;
    }

    // A readback dispatched before the roster was emptied can land after it, and its shape is the
    // only thing that says whether it still describes two lanes.
    if (results.pEntities->size() != kLaneCount * kOrbitPathSampleCount || results.states.size() != results.pEntities->size())
    {
        return;
    }
    m_LastAppliedResultsTime = results.time;

    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        const size_t offset = lane * kOrbitPathSampleCount;
        const std::span<const SGP4StepOutput> states(results.states.data() + offset, kOrbitPathSampleCount);

        const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);
        m_LanePoints[lane] = BuildOrbitPathPoints(states, range);
        m_PointsEntities[lane] = (*results.pEntities)[offset];

        // The anchor is inside the range whenever the range is not empty, by construction.
        m_AnchorArcLengths[lane] = range.count > 0 ? m_LanePoints[lane][kOrbitPathAnchorIndex - range.first].arcLength : 0.0f;

        if (m_LanePoints[lane].empty())
        {
            continue;
        }

        m_PointsScratch.clear();
        m_PointsScratch.reserve(m_LanePoints[lane].size());
        for (const OrbitPathPoint& point : m_LanePoints[lane])
        {
            m_PointsScratch.push_back(OrbitPathPointGPU{ point.position, point.arcLength });
        }

        GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(
            m_PointsBuffers[lane], 0, m_PointsScratch.data(), m_PointsScratch.size() * sizeof(OrbitPathPointGPU));
    }
}

void OrbitPathRenderSystem::UpdateUniforms(entt::registry& registry)
{
    SpaceObjectRenderSystem* pSpaceObjectRenderSystem = GetActiveScene()->GetSystem<SpaceObjectRenderSystem>();
    if (pSpaceObjectRenderSystem == nullptr)
    {
        return;
    }

    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        if (!IsLaneDrawable(lane))
        {
            continue;
        }

        const std::vector<OrbitPathPoint>& points = m_LanePoints[lane];

        // Against the path actually drawn, so a truncated one keeps its dashes rather than
        // stretching them over what is left.
        const float totalArcLength = points.back().arcLength;
        const float dashLengthKm = totalArcLength > 0.0f ? totalArcLength / kDashesPerOrbit : 1.0f;

        const glm::vec3 color = pSpaceObjectRenderSystem->GetSpaceObjectColor(m_Lanes[lane]).AsVec3();
        const float alpha = (lane == kSelectedLane) ? kSelectedAlpha : kHoveredAlpha;

        for (size_t pass = 0; pass < kPassCount; pass++)
        {
            const float passAlpha = (pass == kOccludedPass) ? alpha * kOccludedAlphaScale : alpha;

            const OrbitPathUniforms uniforms{
                .color = glm::vec4(color, passAlpha),
                .halfWidthPixels = kRibbonHalfWidthPixels,
                .dashLengthKm = dashLengthKm,
                .anchorArcLength = m_AnchorArcLengths[lane],
                .pointCount = static_cast<uint32_t>(points.size())
            };

            GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(
                m_UniformsBuffer, UniformSlot(lane, pass) * kUniformSlotStride, &uniforms, sizeof(OrbitPathUniforms));
        }
    }
}

// A lane is drawn only once the readback has caught up with the selection, which takes a couple of
// frames.
bool OrbitPathRenderSystem::IsLaneDrawable(size_t lane) const
{
    if (m_LanePoints[lane].size() < 2)
    {
        return false;
    }

    if (m_Lanes[lane] == NullEntityHandle || m_Lanes[lane] != m_PointsEntities[lane])
    {
        return false;
    }

    // Hovering the selected object would otherwise blend its path over itself.
    if (lane == kHoveredLane && m_Lanes[kHoveredLane] == m_Lanes[kSelectedLane])
    {
        return false;
    }

    return true;
}

void OrbitPathRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (!m_VisiblePipeline || !m_OccludedPipeline)
    {
        return;
    }

    for (size_t lane = 0; lane < kLaneCount; lane++)
    {
        if (!IsLaneDrawable(lane))
        {
            continue;
        }

        // Two vertices per point, expanded either side of the centreline by the vertex shader.
        const uint32_t vertexCount = static_cast<uint32_t>(m_LanePoints[lane].size() * 2);

        renderPass.SetPipeline(m_OccludedPipeline);
        renderPass.SetBindGroup(1, m_BindGroups[UniformSlot(lane, kOccludedPass)]);
        renderPass.Draw(vertexCount);

        renderPass.SetPipeline(m_VisiblePipeline);
        renderPass.SetBindGroup(1, m_BindGroups[UniformSlot(lane, kVisiblePass)]);
        renderPass.Draw(vertexCount);
    }
}

} // namespace WingsOfSteel
