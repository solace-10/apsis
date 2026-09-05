#include <cmath>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/geometric.hpp>

#include "gpu/compute_harness.hpp"
#include "reference/sgp4_reference.hpp"
#include "render/sgp4_compute_pass.hpp"
#include "space/sgp4.hpp"

using namespace WingsOfSteel;

namespace
{

const std::string kShaderFile = std::string(APSIS_SHADER_DIR) + "/sgp4.wgsl";
const std::string kTleFile = std::string(APSIS_TEST_DATA_DIR) + "/SGP4-VER.TLE";

// Must match @workgroup_size in sgp4.wgsl, which is also what SGP4ComputePass rounds its dispatch
// up to. If the two disagree the elements past the end of the short dispatch are never written,
// which shows up below as a position of zero rather than as a wrong one.
constexpr uint32_t kWorkgroupSize = 64;

// Measured, not chosen.
//
// The worst observed across the whole set is 295 m and 2.3e-4 km/s, on 00005 at t = 1800 minutes.
// The bounds sit a little over three times above that rather than the order of magnitude this
// suite usually allows, because unlike everything else here the number is the point: a tolerance
// loose enough to be comfortable would be loose enough to hide the regression it exists to catch.
// If it ever fails on different hardware, the margin is the thing to revisit - f32 results are not
// required to agree between drivers.
constexpr double kPositionToleranceKm = 1.0;
constexpr double kVelocityToleranceKmPerSecond = 0.001;

// One element of the dispatch, and what the CPU makes of the same inputs.
struct Sample
{
    std::string satnum;
    double tsince{ 0.0 };
    SGP4StepInput input;
    float time{ 0.0f };
    SGP4Position expected;
};

// Every near-earth element set in the verification file, at every time it asks to be propagated
// to, stopping where the propagation does. The same walk sgp4_step_tests.cpp makes, flattened into
// one dispatch so that the shader answers for all of them at once.
std::vector<Sample> MakeSamples()
{
    std::vector<Sample> samples;

    for (const Test::VerificationCase& verificationCase : Test::LoadVerificationCases(kTleFile))
    {
        if (verificationCase.satrec.method != 'n')
        {
            continue;
        }

        const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));

        for (const double tsince : Test::VerificationTimes(verificationCase))
        {
            // Both sides are given the same f32-representable time, so that what is measured is
            // the arithmetic rather than the rounding of the time going in. That rounding is a
            // real effect and a real hazard - at ten days from epoch an f32 tsince is worth
            // several hundred metres of along-track error - but it is a separate one, and it
            // belongs to whoever decides how time reaches the GPU.
            const float tsinceFloat = static_cast<float>(tsince);
            const SGP4Position expected = SGP4Step(elementSet, static_cast<double>(tsinceFloat));

            samples.push_back(Sample{
                verificationCase.satnum,
                tsince,
                MakeSGP4StepInput(elementSet),
                tsinceFloat,
                expected });

            if (expected.error != SGP4Error::None)
            {
                break;
            }
        }
    }

    return samples;
}

} // namespace

// What f32 costs, which is the one thing none of the rest of this suite can say.
//
// Both halves of SGP4 now exist in double and both agree with Vallado exactly, so the shader is
// the only place precision can be lost - and it is compared against SGP4Step() rather than against
// the reference for exactly that reason. A comparison against the reference would be measuring
// transcription mistakes and precision together; this one has already had the first ruled out.
//
// It also carries everything the echo case it replaced used to: the struct C++ uploads is the
// struct WGSL reads, the bindings agree, and the tail of a rounded-up dispatch is guarded - 162
// samples is not a multiple of 64, so the last workgroup runs threads past the end.
TEST_CASE("The compute shader propagates as accurately as f32 allows", "[space][sgp4][gpu]")
{
    Test::ComputeHarness harness;
    if (!harness.IsAvailable())
    {
        SKIP("No WebGPU device on this machine: " << harness.GetUnavailableReason());
    }

    const std::vector<Sample> samples = MakeSamples();
    REQUIRE(samples.size() == 162);
    REQUIRE(samples.size() % kWorkgroupSize != 0);

    // The same two buffers the pass uploads, bound the same way: coefficients that would change
    // only when the roster does, and one time per object.
    std::vector<SGP4StepInput> inputs;
    std::vector<float> times;
    inputs.reserve(samples.size());
    times.reserve(samples.size());
    for (const Sample& sample : samples)
    {
        inputs.push_back(sample.input);
        times.push_back(sample.time);
    }

    const wgpu::ShaderModule shaderModule = harness.CompileFromFile(kShaderFile);
    const std::vector<SGP4StepOutput> states = harness.Dispatch<SGP4StepOutput>(shaderModule, "computeSGP4", inputs, times, kWorkgroupSize);
    REQUIRE(states.size() == samples.size());

    double worstPosition = 0.0;
    double worstVelocity = 0.0;
    std::string worstAt;

    for (size_t i = 0; i < samples.size(); i++)
    {
        const Sample& sample = samples[i];
        INFO("satellite " << sample.satnum << " at t = " << sample.tsince << " minutes");

        // Where the propagation gives up has to agree as well as where the object is. The shader
        // has no way to return an enum, so it writes the number instead.
        REQUIRE(states[i].error == static_cast<uint32_t>(sample.expected.error));

        // A decayed orbit still has a position; the other failures give up before there is one.
        if (sample.expected.error != SGP4Error::None && sample.expected.error != SGP4Error::Decayed)
        {
            continue;
        }

        const double positionError = glm::length(glm::dvec3(states[i].position) - sample.expected.position);
        const double velocityError = glm::length(glm::dvec3(states[i].velocity) - sample.expected.velocity);

        if (positionError > worstPosition)
        {
            worstPosition = positionError;
            worstAt = sample.satnum + " at t = " + std::to_string(sample.tsince);
        }
        worstVelocity = std::max(worstVelocity, velocityError);
    }

    UNSCOPED_INFO("worst position error " << worstPosition << " km, worst velocity error " << worstVelocity << " km/s, worst at " << worstAt);

    CHECK(worstPosition < kPositionToleranceKm);
    CHECK(worstVelocity < kVelocityToleranceKmPerSecond);
}
