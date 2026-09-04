#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "gpu/compute_harness.hpp"
#include "render/sgp4_compute_pass.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;

namespace
{

const std::string kShaderFile = std::string(APSIS_SHADER_DIR) + "/sgp4.wgsl";

// Must match @workgroup_size in sgp4.wgsl, which is also what SGP4ComputePass rounds its
// dispatch up to. If the two disagree the elements past the end of the short dispatch are
// never written, which the cases below see as zeroed positions rather than as a wrong
// number - so the disagreement is caught either way.
constexpr uint32_t kWorkgroupSize = 64;

// Not a multiple of the workgroup size, on purpose: the dispatch is rounded up, so the last
// workgroup runs threads past the end of the array and the shader has to say so.
constexpr size_t kElementCount = 100;

std::vector<OrbitalElementsInput> MakeElements()
{
    std::vector<OrbitalElementsInput> elements;
    elements.reserve(kElementCount);

    // Values chosen only so that every field of every element is distinct: an element that
    // read its neighbour's slot, or read the right slot at the wrong offset, would land on
    // a number belonging to something else rather than on a plausible one.
    for (size_t i = 0; i < kElementCount; i++)
    {
        const float index = static_cast<float>(i);
        elements.push_back(OrbitalElementsInput{
            .meanMotion = 1.0f + index * 0.125f,
            .eccentricity = 2.0f + index * 0.125f,
            .inclination = 3.0f + index * 0.125f,
            .raan = 4.0f + index * 0.125f,
            .argumentOfPericenter = 5.0f + index * 0.125f,
            .meanAnomaly = 6.0f + index * 0.125f });
    }

    return elements;
}

} // namespace

// The propagator itself is not written yet. What can be pinned now is everything around it:
// that the shader compiles, that the struct C++ uploads is the struct WGSL reads, that the
// bindings the pass declares are the bindings the shader declares, that the tail of a
// rounded-up dispatch is guarded, and that what the GPU wrote comes back intact.
//
// sgp4.wgsl echoes three of its inputs into the output for exactly this reason. When the
// propagation lands the echo goes with it, and this case is replaced by the comparison
// against the reference propagator in sgp4_tests.cpp.
TEST_CASE("The compute shader receives the orbital elements it was given", "[space][sgp4][gpu]")
{
    Test::ComputeHarness harness;
    if (!harness.IsAvailable())
    {
        SKIP("No WebGPU device on this machine: " << harness.GetUnavailableReason());
    }

    const wgpu::ShaderModule shaderModule = harness.CompileFromFile(kShaderFile);
    const std::vector<OrbitalElementsInput> elements = MakeElements();
    const std::vector<OrbitalElementsOutput> positions = harness.Dispatch<OrbitalElementsOutput>(shaderModule, "computeSGP4", elements, kWorkgroupSize);

    REQUIRE(positions.size() == elements.size());

    for (size_t i = 0; i < positions.size(); i++)
    {
        INFO("element " << i);

        // Exact rather than approximate: these values went to the GPU as f32 and came back
        // as f32 without arithmetic in between, so anything but equality is a plumbing
        // fault rather than a precision one.
        REQUIRE(positions[i].position.x == elements[i].meanMotion);
        REQUIRE(positions[i].position.y == elements[i].eccentricity);
        REQUIRE(positions[i].position.z == elements[i].inclination);
    }
}
