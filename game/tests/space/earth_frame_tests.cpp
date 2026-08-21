#include <chrono>
#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include "space/earth_frame.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;

namespace
{

// J2000.0, which for the UT1 form of the GMST series is JD 2451545.0 - 2000-01-01 12:00.
constexpr double kJ2000UnixSeconds = 946728000.0;

std::chrono::system_clock::time_point UnixSeconds(double seconds)
{
    return std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(seconds)));
}

double WrapDegrees(double degrees)
{
    degrees = std::fmod(degrees + 180.0, 360.0);
    return degrees < 0.0 ? degrees + 180.0 : degrees - 180.0;
}

// The geographic longitude the surface texture shows at a given model-space direction, recovered
// from the texture coordinate the mesh generator would give that vertex.
double SurfaceLongitudeAt(const glm::dvec3& modelDirection)
{
    return (static_cast<double>(DirectionToSurfaceUV(glm::vec3(modelDirection)).x) - 0.5) * 360.0;
}

double SurfaceLatitudeAt(const glm::dvec3& modelDirection)
{
    return (0.5 - static_cast<double>(DirectionToSurfaceUV(glm::vec3(modelDirection)).y)) * 180.0;
}

glm::dvec3 DirectionAt(double latitudeDegrees, double azimuthDegrees)
{
    const double lat = glm::radians(latitudeDegrees);
    const double az = glm::radians(azimuthDegrees);
    return glm::dvec3(std::cos(lat) * std::cos(az), std::sin(lat), std::cos(lat) * std::sin(az));
}

} // namespace

TEST_CASE("GMST matches its defining epoch and rate", "[space][earth_frame]")
{
    // The two constants the series is built from, checked at the only instant where they can be
    // read off directly. If either is ever mistyped this is what catches it.
    REQUIRE_THAT(glm::degrees(CalculateGMST(UnixSeconds(kJ2000UnixSeconds))), WithinAbs(280.46061837, 1e-9));

    const double oneDayLater = glm::degrees(CalculateGMST(UnixSeconds(kJ2000UnixSeconds + 86400.0)));
    REQUIRE_THAT(WrapDegrees(oneDayLater - 280.46061837), WithinAbs(360.98564736629 - 360.0, 1e-9));
}

TEST_CASE("GMST stays within one turn", "[space][earth_frame]")
{
    // Including well before J2000, where an unguarded fmod returns a negative angle.
    for (double days : { -20000.0, -1.5, 0.0, 0.25, 9000.0 })
    {
        const double gmst = CalculateGMST(UnixSeconds(kJ2000UnixSeconds + days * 86400.0));
        REQUIRE(gmst >= 0.0);
        REQUIRE(gmst < 2.0 * glm::pi<double>());
    }
}

TEST_CASE("The world swizzle is its own inverse's inverse", "[space][earth_frame]")
{
    const glm::dvec3 eci(1.0, 2.0, 3.0);

    // World is y-up, so the ECI spin axis has to come out as world up.
    REQUIRE(ECIToWorld(glm::dvec3(0.0, 0.0, 1.0)) == glm::dvec3(0.0, 1.0, 0.0));

    const glm::dvec3 roundTripped = WorldToECI(ECIToWorld(eci));
    REQUIRE_THAT(roundTripped.x, WithinAbs(eci.x, 1e-12));
    REQUIRE_THAT(roundTripped.y, WithinAbs(eci.y, 1e-12));
    REQUIRE_THAT(roundTripped.z, WithinAbs(eci.z, 1e-12));
}

TEST_CASE("The surface mapping puts Greenwich on model +X and north at v zero", "[space][earth_frame]")
{
    REQUIRE_THAT(SurfaceLongitudeAt(glm::dvec3(1.0, 0.0, 0.0)), WithinAbs(0.0, 1e-5));
    REQUIRE_THAT(static_cast<double>(DirectionToSurfaceUV(glm::vec3(0.0f, 1.0f, 0.0f)).y), WithinAbs(0.0, 1e-6));

    // East has to run the way the planet turns. A positive rotation about world +Y carries +Z to
    // +X, so east of Greenwich is towards -Z; get this backwards and the map is mirrored.
    REQUIRE(SurfaceLongitudeAt(glm::dvec3(1.0, 0.0, -1.0)) > 0.0);
}

TEST_CASE("An unrotated planet sits at a sidereal time of 90 degrees", "[space][earth_frame]")
{
    // The quarter turn is the whole subtlety of CalculatePlanetRotation(): the mesh's prime
    // meridian is on model +X, and ECIToWorld() puts world +X on right ascension 90 degrees. This
    // pins that identity is the *correct* orientation at GMST 90, not at GMST 0 - which is exactly
    // what was wrong when the planet was never rotated at all.
    const glm::dmat4 atNinety = CalculatePlanetRotation(glm::half_pi<double>());
    REQUIRE_THAT(atNinety[0][0], WithinAbs(1.0, 1e-12));
    REQUIRE_THAT(atNinety[2][2], WithinAbs(1.0, 1e-12));
    REQUIRE_THAT(atNinety[0][2], WithinAbs(0.0, 1e-12));
}

TEST_CASE("A satellite is drawn over the ground it is reported to be over", "[space][earth_frame]")
{
    // The regression this suite exists for.
    //
    // Three separate conventions have to agree: the mesh's texture mapping, the planet's model
    // matrix, and the swizzle that places satellites. Nothing but this relates them, and when they
    // disagreed the error was GMST - 90 degrees - up to a third of the way round the planet,
    // drifting 15 degrees an hour, and briefly small enough once a sidereal day to look plausible.
    //
    // Read forwards: take a point on the mesh, ask the texture what ground it shows, rotate it into
    // world space, then ask which satellite would be drawn exactly there. That satellite's reported
    // sub-point must be the ground we started from.
    for (int hour = 0; hour < 24; hour++)
    {
        const double gmst = glm::radians(hour * 15.0);
        const glm::dmat4 rotation = CalculatePlanetRotation(gmst);

        for (double latitude : { -67.0, -23.5, 0.0, 38.9, 71.0 })
        {
            for (double azimuth : { -175.0, -90.0, 0.0, 45.0, 128.0 })
            {
                const glm::dvec3 modelDirection = DirectionAt(latitude, azimuth);
                const glm::dvec3 world(rotation * glm::dvec4(modelDirection, 1.0));
                const glm::dvec2 reported = ECIToLatLon(WorldToECI(world), gmst);

                INFO("gmst=" << hour * 15 << "deg lat=" << latitude << " az=" << azimuth);
                REQUIRE_THAT(WrapDegrees(reported.y - SurfaceLongitudeAt(modelDirection)), WithinAbs(0.0, 1e-5));

                // Latitude is invariant under a spin about the polar axis. This is what fails if
                // the planet is ever rotated about the wrong axis.
                REQUIRE_THAT(reported.x - SurfaceLatitudeAt(modelDirection), WithinAbs(0.0, 1e-5));
            }
        }
    }
}

TEST_CASE("Sub-satellite longitude tracks the planet turning underneath", "[space][earth_frame]")
{
    // A fixed inertial direction should have the ground slide westwards beneath it as the Earth
    // turns east, at very nearly fifteen degrees an hour.
    const glm::dvec3 fixedInertialDirection(1.0, 0.0, 0.0);

    const double before = ECIToLatLon(fixedInertialDirection, 0.0).y;
    const double after = ECIToLatLon(fixedInertialDirection, glm::radians(15.0)).y;

    REQUIRE_THAT(WrapDegrees(after - before), WithinAbs(-15.0, 1e-9));
}
