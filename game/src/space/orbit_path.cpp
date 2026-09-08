#include <cmath>

#include <glm/geometric.hpp>

#include "space/earth_frame.hpp"
#include "space/orbit_path.hpp"
#include "space/sgp4.hpp"

namespace WingsOfSteel
{

namespace
{

    constexpr double kMinutesPerDay = 1440.0;

    bool IsUsable(const SGP4StepOutput& state)
    {
        return state.error == static_cast<uint32_t>(SGP4Error::None);
    }

    // A sample the path may end on but cannot continue through.
    bool IsTerminator(const SGP4StepOutput& state)
    {
        return state.error == static_cast<uint32_t>(SGP4Error::Decayed);
    }

} // namespace

double OrbitalPeriodMinutes(double meanMotionRevPerDay)
{
    if (meanMotionRevPerDay <= 0.0)
    {
        return 0.0;
    }

    return kMinutesPerDay / meanMotionRevPerDay;
}

std::vector<float> BuildOrbitPathSampleTimes(double tsinceAnchorMinutes, double periodMinutes, size_t sampleCount)
{
    std::vector<float> times;
    times.reserve(sampleCount);

    if (sampleCount == 0)
    {
        return times;
    }
    else if (sampleCount == 1)
    {
        times.push_back(static_cast<float>(tsinceAnchorMinutes));
        return times;
    }

    // Gaps rather than samples, so the ends land exactly half a period either side of the anchor.
    const size_t anchorIndex = sampleCount / 2;
    const double step = periodMinutes / static_cast<double>(sampleCount - 1);

    for (size_t i = 0; i < sampleCount; i++)
    {
        const double offset = (static_cast<double>(i) - static_cast<double>(anchorIndex)) * step;
        times.push_back(static_cast<float>(tsinceAnchorMinutes + offset));
    }

    return times;
}

OrbitPathRange TruncateAtPropagationErrors(std::span<const SGP4StepOutput> states, size_t anchorIndex)
{
    if (anchorIndex >= states.size() || !IsUsable(states[anchorIndex]))
    {
        return OrbitPathRange{};
    }

    size_t first = anchorIndex;
    while (first > 0)
    {
        const SGP4StepOutput& previous = states[first - 1];
        if (!IsUsable(previous))
        {
            if (IsTerminator(previous))
            {
                first--;
            }
            break;
        }
        first--;
    }

    size_t last = anchorIndex;
    while (last + 1 < states.size())
    {
        const SGP4StepOutput& next = states[last + 1];
        if (!IsUsable(next))
        {
            if (IsTerminator(next))
            {
                last++;
            }
            break;
        }
        last++;
    }

    return OrbitPathRange{ first, last - first + 1 };
}

std::vector<OrbitPathPoint> BuildOrbitPathPoints(std::span<const SGP4StepOutput> states, const OrbitPathRange& range)
{
    std::vector<OrbitPathPoint> points;

    if (range.count == 0 || range.first + range.count > states.size())
    {
        return points;
    }

    points.reserve(range.count);

    float arcLength = 0.0f;
    glm::vec3 previousPosition(0.0f);

    for (size_t i = 0; i < range.count; i++)
    {
        const glm::vec3 position(ECIToWorld(glm::dvec3(states[range.first + i].position)));

        if (i > 0)
        {
            arcLength += glm::length(position - previousPosition);
        }

        points.push_back(OrbitPathPoint{ position, arcLength });
        previousPosition = position;
    }

    return points;
}

} // namespace WingsOfSteel
