#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <render/pass/pass.hpp>
#include <resources/resource_shader.hpp>

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

// Propagates every space object's position on the GPU and copies the results back for
// the CPU to consume.
//
// The readback is asynchronous and cannot be made otherwise: mapping only completes
// once the GPU has caught up, and on the web there is no way to block on it at all.
// Positions therefore lag the dispatch that produced them by a couple of frames, which
// is why GetPropagatedPositionsTime() reports the instant they are valid for rather
// than leaving the caller to assume a fixed number of frames.
DECLARE_SMART_PTR(SGP4ComputePass);
class SGP4ComputePass : public Pass
{
public:
    SGP4ComputePass();
    ~SGP4ComputePass();

    void Execute(wgpu::CommandEncoder& encoder) override;

    // False until the shader has loaded and the pipeline exists. SetOrbitalElements()
    // is inert before this point, as the bind group layout comes from the pipeline.
    bool IsReady() const { return m_ComputePipeline != nullptr; }

    void SetOrbitalElements(const std::vector<OrbitalElementsInput>& orbitalElements);

    // Positions in km, in ECI, from the most recent completed readback. Empty until the
    // first one lands, and never guaranteed to correspond to the most recent call to
    // SetOrbitalElements() - see GetPropagatedPositionsTime().
    const std::vector<glm::vec3>& GetPropagatedPositions() const { return m_pReadback->positions; }
    std::chrono::system_clock::time_point GetPropagatedPositionsTime() const { return m_pReadback->positionsTime; }

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
        std::chrono::system_clock::time_point dispatchTime; // Of the copy currently in flight.
        std::vector<glm::vec3> positions; // Of the last completed readback.
        std::chrono::system_clock::time_point positionsTime;
    };

    void CreateComputePipeline();
    void CreateStorageBuffers(size_t numOrbitalElements);
    void RequestReadbackMap();

    static constexpr uint32_t kWorkgroupSize = 64;

    ResourceShaderSharedPtr m_pShader;
    wgpu::ComputePipeline m_ComputePipeline;
    wgpu::Buffer m_OrbitalElementsBuffer;
    wgpu::Buffer m_PropagatedPositionsBuffer;
    wgpu::BindGroup m_BindGroup;
    std::shared_ptr<Readback> m_pReadback;
    size_t m_NumOrbitalElements{ 0 };
};

} // namespace WingsOfSteel
