#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <vector>

#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <resources/resource_shader.hpp>
#include <scene/entity.hpp>
#include <scene/systems/system.hpp>

#include "render/sgp4_compute_pass.hpp"
#include "space/orbit_path.hpp"

namespace WingsOfSteel
{

// Draws the orbit of the selected space object, and of the one under the cursor, as a ribbon one
// revolution long: solid behind the object, dashed ahead of it.
//
// sgp4.wgsl assumes nothing about its elements being distinct, so a path is one element set
// repeated across a run of the roster with a different time in each slot.
//
// A second SGP4ComputePass rather than a share of the main one, whose roster is every visible
// object and is re-packed whole whenever it changes.
class OrbitPathRenderSystem : public System
{
public:
    OrbitPathRenderSystem();
    ~OrbitPathRenderSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    // Called from OrbitOverlayRenderPass, which tests against the depth the scene left behind.
    void Render(wgpu::RenderPassEncoder& renderPass);

private:
    // Mirrors OrbitPathUniforms in orbit_path.wgsl.
    struct OrbitPathUniforms
    {
        glm::vec4 color; // rgb, with the alpha the whole ribbon is scaled by.
        float halfWidthPixels;
        float dashLengthKm;
        float anchorArcLength; // Where along the path the object itself is.
        uint32_t pointCount;
    };

    // Mirrors OrbitPathPoint in orbit_path.wgsl, and deliberately not OrbitPathPoint itself: a
    // field added to that one must not silently change what the shader reads.
    struct OrbitPathPointGPU
    {
        glm::vec3 position;
        float arcLength;
    };

    static_assert(sizeof(OrbitPathUniforms) == 32, "OrbitPathUniforms must match its WGSL counterpart");
    static_assert(sizeof(OrbitPathPointGPU) == 16, "OrbitPathPointGPU must match its WGSL counterpart");

    // A lane with no object of its own propagates the other's rather than shortening the roster,
    // so the element count never changes and SGP4ComputePass never rebuilds its buffers.
    static constexpr size_t kLaneCount = 2;
    static constexpr size_t kSelectedLane = 0;
    static constexpr size_t kHoveredLane = 1;

    // Each lane is drawn twice: once in front of the planet and once, faintly, behind it. Without
    // the second the far half of every orbit vanishes, which is much harder to read.
    static constexpr size_t kPassCount = 2;
    static constexpr size_t kVisiblePass = 0;
    static constexpr size_t kOccludedPass = 1;

    void CreateRenderPipelines();
    wgpu::RenderPipeline CreateRenderPipeline(const char* pLabel, wgpu::CompareFunction depthCompare);
    void CreateBindGroupLayout();
    void CreateBuffers();
    void HandleShaderInjection();

    bool CanDrawPath(entt::registry& registry, EntityHandle entityHandle) const;
    void ResolveTrackedObjects(entt::registry& registry);
    void UpdateRoster(entt::registry& registry);
    void UpdateTimes(entt::registry& registry, const std::chrono::system_clock::time_point& instant);
    void ApplyPropagatedPaths();
    void UpdateUniforms(entt::registry& registry);
    bool IsLaneDrawable(size_t lane) const;

    static size_t UniformSlot(size_t lane, size_t pass) { return lane * kPassCount + pass; }

    SGP4ComputePassSharedPtr m_pComputePass;
    ResourceShaderSharedPtr m_pShader;
    std::optional<SignalId> m_ShaderInjectionSignalId;

    wgpu::RenderPipeline m_VisiblePipeline;
    wgpu::RenderPipeline m_OccludedPipeline;
    wgpu::BindGroupLayout m_BindGroupLayout;
    wgpu::Buffer m_UniformsBuffer;
    std::array<wgpu::Buffer, kLaneCount> m_PointsBuffers;
    std::array<wgpu::BindGroup, kLaneCount * kPassCount> m_BindGroups;

    // The objects whose paths should be on screen right now. Either may be null.
    std::array<EntityHandle, kLaneCount> m_Lanes{ NullEntityHandle, NullEntityHandle };

    // What was actually uploaded, after a lane without an object borrowed the other's element set.
    std::array<EntityHandle, kLaneCount> m_DispatchLanes{ NullEntityHandle, NullEntityHandle };

    // Whose samples each lane's points were built from. Not necessarily m_Lanes: the readback runs
    // a couple of frames behind the selection, and a path is drawn only once the two agree.
    std::array<EntityHandle, kLaneCount> m_PointsEntities{ NullEntityHandle, NullEntityHandle };

    std::array<std::vector<OrbitPathPoint>, kLaneCount> m_LanePoints;
    std::array<float, kLaneCount> m_AnchorArcLengths{ 0.0f, 0.0f };

    EntityRosterSharedPtr m_pRoster;
    EntityRoster m_RosterScratch;
    std::vector<SGP4StepInput> m_OrbitalElements;
    std::vector<float> m_Times;
    std::vector<OrbitPathPointGPU> m_PointsScratch;

    std::chrono::system_clock::time_point m_LastAppliedResultsTime;
};

} // namespace WingsOfSteel
