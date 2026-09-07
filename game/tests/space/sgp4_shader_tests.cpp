#include <algorithm>
#include <cmath>
#include <cstdlib>
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

// How far from its epoch the application can ever ask for an element set, and so the range the
// deep-space bound below is set over. spacetrack.py admits nothing with an epoch over 30 days old
// and clear_stale_objects() trims what stops being refreshed after 3, so 33 days is the reach;
// sgp4.cpp's kResonanceMaxSteps is derived from the same number.
constexpr double kCatalogueReachMinutes = 33.0 * 24.0 * 60.0;

// Deep space, measured the same way and with the same headroom, but it needs two bounds rather
// than one because its error distribution has a long tail and a single worst case would be set by
// that tail and blind to everything else.
//
// Over the 439 in-reach samples: median 57 m, p90 557 m, p99 6.6 km, worst 56.0 km. Only 24 exceed
// a kilometre and only one exceeds ten. The tail is eccentricity, not the deep-space code and not
// elapsed time: the worst is 33333 at t = 20 minutes, twenty minutes from its own epoch, and the
// eleven behind it are all 23333. Those two are e = 0.995 and e = 0.973 - all but parabolic, where
// position is violently sensitive to the eccentric anomaly and f32 has nothing left to give. The
// file carries them because they are hard; 23333's own comment says Kepler fails past about 200
// minutes. Nothing shaped like that survives in an Earth-orbit catalogue in any number.
//
// So the worst-case bound is generous by necessity, and p90 is pinned beside it because that is
// what a transcription regression would actually move - a wrong term shifts every sample, not one.
constexpr double kDeepSpacePositionToleranceKm = 200.0;
constexpr double kDeepSpaceP90ToleranceKm = 2.0;
constexpr double kDeepSpaceVelocityToleranceKmPerSecond = 0.0025;

// 20413's second entry alone, three and a half years past its epoch. f32 error grows with t, so
// this is large by construction; bounded so a regression still shows.
constexpr double kBeyondReachPositionToleranceKm = 275.0;
constexpr double kBeyondReachVelocityToleranceKmPerSecond = 0.35;

// One element of the dispatch, and what the CPU makes of the same inputs.
struct Sample
{
    std::string satnum;
    double tsince{ 0.0 };
    SGP4StepInput input;
    float time{ 0.0f };
    SGP4Position expected;
    bool deepSpace{ false };
};

// Every near-earth element set in the verification file, at every time it asks to be propagated
// to, stopping where the propagation does. The same walk sgp4_step_tests.cpp makes, flattened into
// one dispatch so that the shader answers for all of them at once.
std::vector<Sample> MakeSamples()
{
    std::vector<Sample> samples;

    for (const Test::VerificationCase& verificationCase : Test::LoadVerificationCases(kTleFile))
    {
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
                expected,
                elementSet.method == SGP4Method::DeepSpace });

            if (expected.error != SGP4Error::None)
            {
                break;
            }
        }

        // The divergence check has nowhere else to be exercised: no case in the file reaches it
        // within the range the file asks for. 29141 taken past that range does, and the shader's
        // copy of the check has to agree with ours about it. It contributes an error code and
        // nothing else - a diverged step has no position to compare.
        if (verificationCase.satnum == "29141")
        {
            const float tsinceFloat = 2784.0f;
            const SGP4Position expected = SGP4Step(elementSet, static_cast<double>(tsinceFloat));
            REQUIRE(expected.error == SGP4Error::DragModelDiverged);

            samples.push_back(Sample{
                verificationCase.satnum,
                static_cast<double>(tsinceFloat),
                MakeSGP4StepInput(elementSet),
                tsinceFloat,
                expected });
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
// struct WGSL reads, the bindings agree, and the tail of a rounded-up dispatch is guarded - 163
// samples is not a multiple of 64, so the last workgroup runs threads past the end.
TEST_CASE("The compute shader propagates as accurately as f32 allows", "[space][sgp4][gpu]")
{
    Test::ComputeHarness harness;
    if (!harness.IsAvailable())
    {
        SKIP("No WebGPU device on this machine: " << harness.GetUnavailableReason());
    }

    const std::vector<Sample> samples = MakeSamples();
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

    // Three measurements, not one, because they answer different questions and a single worst case
    // would be set by the last of them and blind to the other two.
    //
    // [0] near earth - item 8's number, which has a history.
    // [1] deep space inside kCatalogueReachMinutes - what the application can actually ask for.
    // [2] deep space beyond it - 20413's second entry alone, propagated three and a half years out
    //     to exercise Lyddane's choice. f32 error grows with t, so this is large by construction
    //     rather than by defect; it is bounded here so a regression still shows, but it is not the
    //     number to quote.
    std::vector<double> positionErrors[3];
    double worstPosition[3] = { 0.0, 0.0, 0.0 };
    double worstVelocity[3] = { 0.0, 0.0, 0.0 };
    std::string worstAt[3];
    size_t counted[3] = { 0, 0, 0 };

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

        size_t branch = 0;
        if (sample.deepSpace)
        {
            branch = std::abs(sample.tsince) <= kCatalogueReachMinutes ? 1 : 2;
        }
        counted[branch]++;
        if (positionError > worstPosition[branch])
        {
            worstPosition[branch] = positionError;
            worstAt[branch] = sample.satnum + " at t = " + std::to_string(sample.tsince);
        }
        worstVelocity[branch] = std::max(worstVelocity[branch], velocityError);
        positionErrors[branch].push_back(positionError);
    }

    // p90 of the in-reach deep-space samples, which is the number that moves if the transcription
    // is wrong rather than merely stretched by an awkward orbit.
    std::sort(positionErrors[1].begin(), positionErrors[1].end());
    REQUIRE(!positionErrors[1].empty());
    const double deepSpaceP90 = positionErrors[1][positionErrors[1].size() * 9 / 10];

    const char* names[3] = { "near earth", "deep space, within reach", "deep space, beyond reach" };
    for (size_t branch = 0; branch < 3; branch++)
    {
        UNSCOPED_INFO(names[branch] << " (" << counted[branch] << " samples): worst position "
                                    << worstPosition[branch] << " km, worst velocity "
                                    << worstVelocity[branch] << " km/s, at " << worstAt[branch]);
    }
    UNSCOPED_INFO("deep space p90 position error " << deepSpaceP90 << " km");

    CHECK(worstPosition[0] < kPositionToleranceKm);
    CHECK(worstVelocity[0] < kVelocityToleranceKmPerSecond);
    CHECK(worstPosition[1] < kDeepSpacePositionToleranceKm);
    CHECK(deepSpaceP90 < kDeepSpaceP90ToleranceKm);
    CHECK(worstVelocity[1] < kDeepSpaceVelocityToleranceKmPerSecond);
    CHECK(worstPosition[2] < kBeyondReachPositionToleranceKm);
    CHECK(worstVelocity[2] < kBeyondReachVelocityToleranceKmPerSecond);
}
