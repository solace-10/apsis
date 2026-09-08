#include <cmath>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "gpu/compute_harness.hpp"
#include "space/orbit_path.hpp"
#include "space/sgp4.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{

const std::string kOrbitPathShaderFile = std::string(APSIS_SHADER_DIR) + "/orbit_path.wgsl";

// A circular orbit at 7000 km, sampled evenly. The truncation cares only about the errors, but
// real positions make the arc length assertions mean something.
constexpr float kTestOrbitRadius = 7000.0f;

SGP4StepOutput MakeState(size_t index, size_t count, SGP4Error error = SGP4Error::None)
{
    const double angle = 2.0 * 3.14159265358979323846 * static_cast<double>(index) / static_cast<double>(count - 1);

    SGP4StepOutput state{};
    state.position = glm::vec3(kTestOrbitRadius * static_cast<float>(std::cos(angle)), kTestOrbitRadius * static_cast<float>(std::sin(angle)), 0.0f);
    state.velocity = glm::vec3(0.0f);
    state.error = static_cast<uint32_t>(error);
    return state;
}

std::vector<SGP4StepOutput> MakeStates(size_t count)
{
    std::vector<SGP4StepOutput> states;
    states.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        states.push_back(MakeState(i, count));
    }
    return states;
}

} // namespace

TEST_CASE("A path is sampled across one whole revolution centred on the current position", "[space][orbit_path]")
{
    const std::vector<float> times = BuildOrbitPathSampleTimes(100.0, 90.0, kOrbitPathSampleCount);

    REQUIRE(times.size() == kOrbitPathSampleCount);

    // Half a period behind and half ahead, so the path closes on itself.
    REQUIRE_THAT(times.front(), WithinAbs(55.0f, 1e-3f));
    REQUIRE_THAT(times.back(), WithinAbs(145.0f, 1e-3f));
    REQUIRE_THAT(times.back() - times.front(), WithinAbs(90.0f, 1e-3f));
}

// The whole reason kOrbitPathSampleCount is odd: the anchor is the pivot the truncation walks out
// from and the phase the dashes are measured against.
TEST_CASE("The current position is a sample rather than a point between two", "[space][orbit_path]")
{
    const std::vector<float> times = BuildOrbitPathSampleTimes(1234.5, 101.0, kOrbitPathSampleCount);

    REQUIRE(kOrbitPathSampleCount % 2 == 1);
    REQUIRE_THAT(times[kOrbitPathAnchorIndex], WithinAbs(1234.5f, 1e-3f));

    // And it really is the middle: as many samples behind it as ahead of it.
    REQUIRE(kOrbitPathAnchorIndex == kOrbitPathSampleCount - 1 - kOrbitPathAnchorIndex);
}

TEST_CASE("A mean motion that cannot describe an orbit yields no period", "[space][orbit_path]")
{
    REQUIRE_THAT(OrbitalPeriodMinutes(15.5), WithinRel(1440.0 / 15.5, 1e-12));
    REQUIRE(OrbitalPeriodMinutes(0.0) == 0.0);
    REQUIRE(OrbitalPeriodMinutes(-1.0) == 0.0);
}

TEST_CASE("A path the propagator placed everywhere is drawn whole", "[space][orbit_path]")
{
    const std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    REQUIRE(range.first == 0);
    REQUIRE(range.count == kOrbitPathSampleCount);
}

// Half a period backwards reaches past the launch of a recently catalogued object, where the drag
// model diverges. The half it has actually flown is still good and must survive.
TEST_CASE("A path stops behind the object without shortening what is ahead of it", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[100].error = static_cast<uint32_t>(SGP4Error::DragModelDiverged);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    // The failed sample has no position in it, so the run starts after it.
    REQUIRE(range.first == 101);
    REQUIRE(range.first + range.count == kOrbitPathSampleCount);
}

TEST_CASE("A path stops ahead of the object without shortening what is behind it", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[400].error = static_cast<uint32_t>(SGP4Error::MeanElementsOutOfRange);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    REQUIRE(range.first == 0);
    REQUIRE(range.first + range.count == 400);
}

// The one error that still carries a position, so keeping that sample ends the trail where the
// object meets the ground rather than one sample short of it.
TEST_CASE("A re-entry ends the path at the point it comes down", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[400].error = static_cast<uint32_t>(SGP4Error::Decayed);
    states[401].error = static_cast<uint32_t>(SGP4Error::Decayed);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    // The first decayed sample is included and the walk stops there, so the second is not.
    REQUIRE(range.first == 0);
    REQUIRE(range.first + range.count == 401);
}

TEST_CASE("A re-entry behind the object ends the path there as well", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[100].error = static_cast<uint32_t>(SGP4Error::Decayed);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    REQUIRE(range.first == 100);
    REQUIRE(range.first + range.count == kOrbitPathSampleCount);
}

TEST_CASE("A path that fails on both sides keeps only the run around the object", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[10].error = static_cast<uint32_t>(SGP4Error::NegativeSemiLatusRectum);
    states[200].error = static_cast<uint32_t>(SGP4Error::DragModelDiverged);
    states[300].error = static_cast<uint32_t>(SGP4Error::MeanMotionNotPositive);
    states[500].error = static_cast<uint32_t>(SGP4Error::Decayed);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    // The nearest failure on each side bounds the run: 201 and 300, not 11 and 500.
    REQUIRE(range.first == 201);
    REQUIRE(range.first + range.count == 300);
}

// That object is one PropagationFailureComponent is about to retire, and half an orbit drawn
// through the last place it was seen would outlive its marker.
TEST_CASE("An object whose current position cannot be placed has no path", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[kOrbitPathAnchorIndex].error = static_cast<uint32_t>(SGP4Error::Decayed);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);

    REQUIRE(range.count == 0);
}

TEST_CASE("A path with nothing to draw is refused rather than clamped", "[space][orbit_path]")
{
    const std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);

    REQUIRE(TruncateAtPropagationErrors({}, 0).count == 0);
    REQUIRE(TruncateAtPropagationErrors(states, kOrbitPathSampleCount).count == 0);
}

TEST_CASE("Arc length accumulates along the path from its first point", "[space][orbit_path]")
{
    const std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);
    const std::vector<OrbitPathPoint> points = BuildOrbitPathPoints(states, range);

    REQUIRE(points.size() == kOrbitPathSampleCount);
    REQUIRE(points.front().arcLength == 0.0f);

    for (size_t i = 1; i < points.size(); i++)
    {
        REQUIRE(points[i].arcLength > points[i - 1].arcLength);
    }

    // A 513-sided polygon inscribed in a 7000 km circle is within a millionth of the circumference.
    REQUIRE_THAT(points.back().arcLength, WithinRel(2.0f * 3.14159265f * kTestOrbitRadius, 1e-4f));
}

// The marker the path runs through was placed by ECIToWorld() too. Anything else would draw the
// orbit next to the object rather than through it.
TEST_CASE("A path is built in the same world space the marker is drawn in", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[kOrbitPathAnchorIndex].position = glm::vec3(1.0f, 2.0f, 3.0f);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);
    const std::vector<OrbitPathPoint> points = BuildOrbitPathPoints(states, range);

    const glm::vec3& anchor = points[kOrbitPathAnchorIndex - range.first].position;
    REQUIRE_THAT(anchor.x, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(anchor.y, WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(anchor.z, WithinAbs(1.0f, 1e-5f));
}

// The ribbon is drawn from the truncated run and the dashes are phased against the anchor's arc
// length, so the two have to be read out of the same array.
TEST_CASE("The object sits at a known distance along a truncated path", "[space][orbit_path]")
{
    std::vector<SGP4StepOutput> states = MakeStates(kOrbitPathSampleCount);
    states[200].error = static_cast<uint32_t>(SGP4Error::DragModelDiverged);

    const OrbitPathRange range = TruncateAtPropagationErrors(states, kOrbitPathAnchorIndex);
    const std::vector<OrbitPathPoint> points = BuildOrbitPathPoints(states, range);

    REQUIRE(range.first == 201);
    REQUIRE(kOrbitPathAnchorIndex - range.first == 55);
    REQUIRE(points.size() == range.count);
    REQUIRE(points[kOrbitPathAnchorIndex - range.first].arcLength > 0.0f);
}

// A render shader is only ever built at runtime, inside a resource callback, so a mistake in one is
// a path that silently fails to appear rather than anything that stops the build. Compiled through
// the engine's own preprocessor, from the tree the game ships from.
TEST_CASE("The orbit path shader compiles", "[space][orbit_path]")
{
    Test::ComputeHarness harness;
    if (!harness.IsAvailable())
    {
        SKIP("No WebGPU device on this machine: " << harness.GetUnavailableReason());
    }

    REQUIRE(harness.CompileFromFile(kOrbitPathShaderFile) != nullptr);
}
