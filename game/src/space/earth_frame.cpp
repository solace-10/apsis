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

glm::dvec3 ECIToECEF(const glm::dvec3& eciPosition, double gmst)
{
    // ECEF - Earth-Centered, Earth-Fixed - shares its origin and spin axis with ECI and differs
    // only by how far the planet has turned, so this is a rotation about z by -GMST.
    const double cosGmst = std::cos(gmst);
    const double sinGmst = std::sin(gmst);

    return glm::dvec3(
        eciPosition.x * cosGmst + eciPosition.y * sinGmst,
        -eciPosition.x * sinGmst + eciPosition.y * cosGmst,
        eciPosition.z);
}

glm::dvec3 ECEFToECI(const glm::dvec3& ecefPosition, double gmst)
{
    return ECIToECEF(ecefPosition, -gmst);
}

glm::dvec3 ECEFToGeodetic(const glm::dvec3& ecefPosition)
{
    const double p = std::sqrt(ecefPosition.x * ecefPosition.x + ecefPosition.y * ecefPosition.y);
    const double z = ecefPosition.z;

    // Bowring's closed form. Latitude appears on both sides of the exact relation, and this is the
    // standard way to get a seed good enough that the iteration below is a formality: it is already
    // sub-millimetre for anything near the surface, and only starts to drift at altitudes far above
    // where we track anything.
    constexpr double kEccentricityPrimeSq = kEarthEccentricitySq / (1.0 - kEarthEccentricitySq);
    const double theta = std::atan2(z * kEarthSemiMajorAxis, p * kEarthSemiMinorAxis);
    const double sinTheta = std::sin(theta);
    const double cosTheta = std::cos(theta);

    double latitude = std::atan2(
        z + kEccentricityPrimeSq * kEarthSemiMinorAxis * sinTheta * sinTheta * sinTheta,
        p - kEarthEccentricitySq * kEarthSemiMajorAxis * cosTheta * cosTheta * cosTheta);

    // Two turns of the exact fixed point, which costs a handful of nanoseconds and removes any
    // question of how the seed behaves out at geostationary altitudes.
    for (int i = 0; i < 2; i++)
    {
        const double sinLatitude = std::sin(latitude);
        const double primeVerticalRadius = kEarthSemiMajorAxis / std::sqrt(1.0 - kEarthEccentricitySq * sinLatitude * sinLatitude);
        latitude = std::atan2(z + kEarthEccentricitySq * primeVerticalRadius * sinLatitude, p);
    }

    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);

    // Not p/cos(latitude) - N, which is the form usually quoted but blows up at the poles. This one
    // is the same quantity rearranged so that nothing is divided by a vanishing cosine.
    const double altitude = p * cosLatitude + z * sinLatitude - kEarthSemiMajorAxis * std::sqrt(1.0 - kEarthEccentricitySq * sinLatitude * sinLatitude);

    return glm::dvec3(
        glm::degrees(latitude),
        glm::degrees(std::atan2(ecefPosition.y, ecefPosition.x)),
        altitude);
}

glm::dvec3 GeodeticToECEF(const glm::dvec3& geodetic)
{
    const double latitude = glm::radians(geodetic.x);
    const double longitude = glm::radians(geodetic.y);
    const double altitude = geodetic.z;

    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);

    // The radius of curvature in the prime vertical: the distance from the point to where its
    // surface normal crosses the spin axis. On a sphere this would just be the radius, and the
    // whole geodetic/geocentric distinction would go away with it.
    const double primeVerticalRadius = kEarthSemiMajorAxis / std::sqrt(1.0 - kEarthEccentricitySq * sinLatitude * sinLatitude);

    return glm::dvec3(
        (primeVerticalRadius + altitude) * cosLatitude * std::cos(longitude),
        (primeVerticalRadius + altitude) * cosLatitude * std::sin(longitude),
        (primeVerticalRadius * (1.0 - kEarthEccentricitySq) + altitude) * sinLatitude);
}

glm::dvec3 ECIToGeodetic(const glm::dvec3& eciPosition, double gmst)
{
    return ECEFToGeodetic(ECIToECEF(eciPosition, gmst));
}

glm::vec2 DirectionToSurfaceUV(const glm::vec3& direction, float semiMajorRadius, float semiMinorRadius)
{
    // The negated atan2 is what makes east run the same way the planet turns. Azimuth atan2(z, x)
    // measures from +X towards +Z, which is the opposite sense to a positive rotation about +Y, so
    // without the flip the map would come out mirrored.
    const float u = 0.5f - std::atan2(direction.z, direction.x) / (2.0f * glm::pi<float>());

    // asin(direction.y) would be the reduced latitude of the vertex the mesh generator places for
    // this direction, and the map is drawn in geodetic. tan(geodetic) = (a/b) * tan(reduced), which
    // is this atan2 with the radii used as the scale factor - and degenerates to asin(direction.y)
    // for a sphere, where a == b.
    const float radiusXZ = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    const float latitude = std::atan2(semiMajorRadius * direction.y, semiMinorRadius * radiusXZ);
    const float v = 0.5f - latitude / glm::pi<float>();

    return glm::vec2(u, v);
}

} // namespace WingsOfSteel
