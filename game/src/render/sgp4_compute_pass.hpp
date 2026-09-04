#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <render/pass/pass.hpp>
#include <resources/resource_shader.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

// One satellite's mean orbital elements, as uploaded to the GPU.
// Mirrors OrbitalElements in sgp4.wgsl - the two must be kept in step.
struct OrbitalElementsInput
{
    float meanMotion;
    float eccentricity;
    float inclination;
    float raan;
    float argumentOfPericenter;
    float meanAnomaly;
    float padding[2];
};

// Mirrors PropagatedPosition in sgp4.wgsl. The trailing float is not slack: a vec3
// aligns to 16 bytes in WGSL, so the struct occupies 16 either way.
struct OrbitalElementsOutput
{
    glm::vec3 position;
    float padding;
};

static_assert(sizeof(OrbitalElementsInput) == 32, "OrbitalElementsInput must match its WGSL counterpart");
static_assert(sizeof(OrbitalElementsOutput) == 16, "OrbitalElementsOutput must match its WGSL counterpart");

// Which entity each element in a dispatch belongs to. Held by shared_ptr and handed to
// the pass alongside the elements, so that a roster stays alive for as long as the
// readback it describes - the results of a dispatch arrive a couple of frames after the
// set of tracked objects may have moved on.
using EntityRoster = std::vector<EntityHandle>;
using EntityRosterSharedPtr = std::shared_ptr<const EntityRoster>;

// The outcome of one completed readback. positions[i] belongs to (*pEntities)[i], and
// the two are only ever assigned together, so they cannot fall out of step.
struct PropagationResults
{
    std::vector<glm::vec3> positions;
    EntityRosterSharedPtr pEntities;
    std::chrono::system_clock::time_point time; // The instant the positions are valid for.
};

// Propagates the position of every tracked space object on the GPU and copies the
// results back for the CPU to consume.
//
// The readback is asynchronous and cannot be made otherwise: mapping only completes once
// the GPU has caught up, and on the web there is no way to block on it at all. Positions
// therefore lag the dispatch that produced them by a couple of frames, which is why the
// results carry both the roster they belong to and the instant they are valid for,
// rather than leaving the caller to assume either.
DECLARE_SMART_PTR(SGP4ComputePass);
class SGP4ComputePass : public Pass
{
public:
    SGP4ComputePass();
    ~SGP4ComputePass();

    void Execute(wgpu::CommandEncoder& encoder) override;

    // False until the shader has loaded and the pipeline exists. SetOrbitalElements() is
    // inert before this point, as the bind group layout comes from the pipeline.
    bool IsReady() const { return m_ComputePipeline != nullptr; }

    // Uploads a new set of objects to propagate. Only needed when the roster changes:
    // orbital elements are static per object, so re-uploading an unchanged set achieves
    // nothing.
    void SetOrbitalElements(const std::vector<OrbitalElementsInput>& orbitalElements, EntityRosterSharedPtr pRoster);

    // Positions in km, in ECI, from the most recent completed readback. Empty until the
    // first one lands, and not necessarily the roster most recently uploaded.
    const PropagationResults& GetResults() const { return *m_pResults; }

private:
    // Held by shared_ptr and captured by value into the map callback, so that a callback
    // arriving after this pass is destroyed - or after a resize has moved on to a new
    // buffer - writes into an orphan nobody reads rather than into freed memory.
    struct Readback
    {
        enum class State
        {
            Idle, // Nothing in flight; free to record a copy into the buffer.
            CopyRecorded, // Copy recorded, but not submitted until the end of the frame.
            MapPending // Map requested; waiting on the callback.
        };

        State state{ State::Idle };
        wgpu::Buffer buffer;
        size_t sizeInBytes{ 0 };

        // Both describe the copy currently in flight.
        std::chrono::system_clock::time_point dispatchTime;
        EntityRosterSharedPtr pDispatchRoster;
    };

    void CreateComputePipeline();
    void CreateStorageBuffers(size_t numOrbitalElements);
    void CreateBindGroup();
    void RequestReadbackMap();
    void HandleShaderInjection();

    static constexpr uint32_t kWorkgroupSize = 64;

    ResourceShaderSharedPtr m_pShader;
    wgpu::ComputePipeline m_ComputePipeline;
    wgpu::Buffer m_OrbitalElementsBuffer;
    wgpu::Buffer m_PropagatedPositionsBuffer;
    wgpu::BindGroup m_BindGroup;
    std::optional<SignalId> m_ShaderInjectionSignalId;
    std::shared_ptr<Readback> m_pReadback;

    // Never replaced, only written to, and captured by every map callback: results
    // outlive the Readback that produced them, and a late callback from an orphaned
    // readback still delivers usable positions because its roster says whose they are.
    std::shared_ptr<PropagationResults> m_pResults;

    EntityRosterSharedPtr m_pRoster;
    size_t m_NumOrbitalElements{ 0 };
};

} // namespace WingsOfSteel
