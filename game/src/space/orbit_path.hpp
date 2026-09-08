#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <glm/vec3.hpp>

#include "render/sgp4_compute_pass.hpp"

namespace WingsOfSteel
{

// Turning a propagated orbit into something that can be drawn. Free functions over the
// propagator's own output struct, so the truncation and the arc length can be exercised without a
// device - see game/tests/space/orbit_path_tests.cpp.
inline constexpr size_t kOrbitPathSampleCount = 513;

inline constexpr size_t kOrbitPathAnchorIndex = kOrbitPathSampleCount / 2;

// Zero for a mean motion that cannot describe an orbit, which callers must treat as "no path"
// rather than dividing by.
double OrbitalPeriodMinutes(double meanMotionRevPerDay);

// Minutes from the object's own epoch, as SGP4ComputePass::SetTimes() takes them. Spans one whole
// period centred on the anchor, so the path closes into a complete revolution.
std::vector<float> BuildOrbitPathSampleTimes(double tsinceAnchorMinutes, double periodMinutes, size_t sampleCount);

struct OrbitPathRange
{
    size_t first{ 0 };
    size_t count{ 0 };
};

// The run of samples around the anchor that can actually be drawn.
//
// Half a period either side of now can reach past what an element set can describe, e.g.
// before a launch or if the object has decayed.
// This function walks outwards from the anchor, so a failure on one side leaves the other whole.
//
// SGP4Error::Decayed is the one error that still carries a position - sgp4.wgsl raises it after
// filling the position in - so a decayed sample is kept as the run's last point, ending the trail
// where the object meets the ground. Every other error zeroes the position and is excluded.
OrbitPathRange TruncateAtPropagationErrors(std::span<const SGP4StepOutput> states, size_t anchorIndex);

struct OrbitPathPoint
{
    glm::vec3 position; // World space, km. See ECIToWorld().
    float arcLength; // km along the path from its first point.
};

// The world-space polyline, with arc length accumulated along it: the dashes are measured in it,
// and its sign relative to the anchor is what separates past from future.
std::vector<OrbitPathPoint> BuildOrbitPathPoints(std::span<const SGP4StepOutput> states, const OrbitPathRange& range);

} // namespace WingsOfSteel
