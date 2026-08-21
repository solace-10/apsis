#include "space/earth_frame.hpp"

#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace WingsOfSteel
{

double CalculateGMST(std::chrono::system_clock::time_point when)
{
    // Days since J2000.0, which for the UT1 form of this series is JD 2451545.0 - that is
    // 2000-01-01 12:00, Unix 946728000. Unix time is close enough to UT1 for this: it ignores leap
    // seconds, so it can be up to a second out, which is well under a hundredth of a degree.
    constexpr double kJ2000UnixSeconds = 946728000.0;
    const double secondsSinceJ2000 = std::chrono::duration<double>(when.time_since_epoch()).count() - kJ2000UnixSeconds;
    const double daysSinceJ2000 = secondsSinceJ2000 / 86400.0;

    // GMST at J2000.0, and the rate: a sidereal day is shorter than a solar one, hence the excess
    // over 360 degrees per day.
    double gmstDegrees = 280.46061837 + 360.98564736629 * daysSinceJ2000;

    gmstDegrees = std::fmod(gmstDegrees, 360.0);
    if (gmstDegrees < 0.0)
    {
        gmstDegrees += 360.0;
    }

    return glm::radians(gmstDegrees);
}

glm::dmat4 CalculatePlanetRotation(double gmst)
{
    // Built in double and narrowed by the caller if need be: the angle runs up to a full turn,
    // where float would quantise it to a few metres of surface error for no reason.
    return glm::rotate(glm::dmat4(1.0), gmst - glm::half_pi<double>(), glm::dvec3(0.0, 1.0, 0.0));
}

glm::dvec3 ECIToWorld(const glm::dvec3& eciPosition)
{
    return glm::dvec3(eciPosition.y, eciPosition.z, eciPosition.x);
}

glm::dvec3 WorldToECI(const glm::dvec3& worldPosition)
{
    return glm::dvec3(worldPosition.z, worldPosition.x, worldPosition.y);
}

glm::dvec2 ECIToLatLon(const glm::dvec3& eciPosition, double gmst)
{
    // ECI to ECEF is a rotation about the spin axis by -GMST. This is the correct pairing for TEME,
    // which is the frame the OMM mean elements are actually expressed in.
    const double cosGmst = std::cos(gmst);
    const double sinGmst = std::sin(gmst);

    const double xEcef = eciPosition.x * cosGmst + eciPosition.y * sinGmst;
    const double yEcef = -eciPosition.x * sinGmst + eciPosition.y * cosGmst;
    const double zEcef = eciPosition.z;

    const double longitude = std::atan2(yEcef, xEcef);

    const double r_xy = std::sqrt(xEcef * xEcef + yEcef * yEcef);
    const double latitude = std::atan2(zEcef, r_xy);

    return glm::dvec2(glm::degrees(latitude), glm::degrees(longitude));
}

glm::vec2 DirectionToSurfaceUV(const glm::vec3& direction)
{
    // The negated atan2 is what makes east run the same way the planet turns. Azimuth atan2(z, x)
    // measures from +X towards +Z, which is the opposite sense to a positive rotation about +Y, so
    // without the flip the map would come out mirrored.
    const float u = 0.5f - std::atan2(direction.z, direction.x) / (2.0f * glm::pi<float>());
    const float v = 0.5f - std::asin(glm::clamp(direction.y, -1.0f, 1.0f)) / glm::pi<float>();
    return glm::vec2(u, v);
}

} // namespace WingsOfSteel
