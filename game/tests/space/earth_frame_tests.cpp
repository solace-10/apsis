#include <chrono>
#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

// A UTC instant, spelled out. The sun cases below are checked against published ephemerides and
// against what the sky actually does on a given afternoon, so the dates have to be legible.
std::chrono::system_clock::time_point Utc(int year, unsigned month, unsigned day, int hour, int minute = 0)
{
    const std::chrono::year_month_day date{ std::chrono::year{ year }, std::chrono::month{ month }, std::chrono::day{ day } };
    return std::chrono::sys_days{ date } + std::chrono::hours{ hour } + std::chrono::minutes{ minute };
}

// The outward normal of the WGS84 ellipsoid at a point on the ground, in ECEF - which way is up
// for someone standing there. Composed out of GeodeticToECEF() rather than restated in closed
// form, for the same reason ModelToECEF() is.
glm::dvec3 GeodeticUp(double latitudeDegrees, double longitudeDegrees)
{
    return glm::normalize(
        GeodeticToECEF(glm::dvec3(latitudeDegrees, longitudeDegrees, 1.0))
        - GeodeticToECEF(glm::dvec3(latitudeDegrees, longitudeDegrees, 0.0)));
}

// How high the Sun stands above the horizon at a point on the ground, in degrees. Negative is
// night. Geometric, so it takes no account of refraction or of the Sun's own radius - both worth
// about half a degree at the horizon and nothing at all further up.
double SolarElevationAt(std::chrono::system_clock::time_point when, double latitudeDegrees, double longitudeDegrees)
{
    const glm::dvec3 sunECEF = ECIToECEF(CalculateSunDirectionECI(when), CalculateGMST(when));
    return glm::degrees(std::asin(glm::dot(GeodeticUp(latitudeDegrees, longitudeDegrees), sunECEF)));
}

// The point on the ground the Sun is directly over, as (latitude, longitude) in degrees.
//
// The Sun is at the zenith where the ellipsoid's normal points straight at it, and geodetic
// latitude is by definition the angle that normal makes with the equator - so the declination is
// the latitude and there is no ellipsoid to solve. ECIToGeodetic() is the wrong tool here: it
// locates the point *at* the coordinates it is handed, and a unit vector is a point 6,377 km
// underneath Greenwich.
glm::dvec2 SubSolarPoint(std::chrono::system_clock::time_point when)
{
    const glm::dvec3 sunECEF = ECIToECEF(CalculateSunDirectionECI(when), CalculateGMST(when));
    return glm::dvec2(glm::degrees(std::asin(sunECEF.z)), glm::degrees(std::atan2(sunECEF.y, sunECEF.x)));
}

// The unit direction the planet mesh generator projects a cube vertex onto, parameterised the way
// the mesh actually parameterises it: by reduced latitude, since the vertex it places is
// (dir.x*a, dir.y*b, dir.z*a) and so dir.y is sin(reduced), not sin(geodetic).
glm::dvec3 MeshDirection(double reducedLatitudeDegrees, double azimuthDegrees)
{
    const double lat = glm::radians(reducedLatitudeDegrees);
    const double az = glm::radians(azimuthDegrees);
    return glm::dvec3(std::cos(lat) * std::cos(az), std::sin(lat), std::cos(lat) * std::sin(az));
}

// The model-space vertex the mesh generator places for that direction: a genuine point on the
// WGS84 ellipsoid, which is what makes the altitude assertions below mean anything.
glm::dvec3 MeshVertex(const glm::dvec3& direction)
{
    return glm::dvec3(
        direction.x * kEarthSemiMajorAxis,
        direction.y * kEarthSemiMinorAxis,
        direction.z * kEarthSemiMajorAxis);
}

// Model space to ECEF, composed out of the functions the renderer actually uses rather than
// restated as a swizzle: at GMST 0 the planet's model matrix and the world relabelling are the
// whole of the difference, and inventing a fourth statement of it here is how these drift apart.
glm::dvec3 ModelToECEF(const glm::dvec3& modelPosition)
{
    return WorldToECI(glm::dvec3(CalculatePlanetRotation(0.0) * glm::dvec4(modelPosition, 1.0)));
}

glm::vec2 SurfaceUVAt(const glm::dvec3& modelDirection)
{
    return DirectionToSurfaceUV(
        glm::vec3(modelDirection),
        static_cast<float>(kEarthSemiMajorAxis),
        static_cast<float>(kEarthSemiMinorAxis));
}

// The geographic position the surface texture shows at a given model-space direction, recovered
// from the texture coordinate the mesh generator would give that vertex.
double SurfaceLongitudeAt(const glm::dvec3& modelDirection)
{
    return (static_cast<double>(SurfaceUVAt(modelDirection).x) - 0.5) * 360.0;
}

double SurfaceLatitudeAt(const glm::dvec3& modelDirection)
{
    return (0.5 - static_cast<double>(SurfaceUVAt(modelDirection).y)) * 180.0;
}

// The angle subtended at the centre of the Earth: the convention geodetic latitude is measured
// against below, and the one that falls out of Cartesian coordinates if nobody asks for geodetic.
double GeocentricLatitudeOf(const glm::dvec3& ecef)
{
    return glm::degrees(std::atan2(ecef.z, std::sqrt(ecef.x * ecef.x + ecef.y * ecef.y)));
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

TEST_CASE("The sun's declination follows the obliquity through the year", "[space][earth_frame]")
{
    // The seasons, which is the coarsest thing a sun can be wrong about. Drop the obliquity term
    // and the declination is flat at zero all year; run the ecliptic longitude backwards and the
    // solstices swap.
    REQUIRE_THAT(SubSolarPoint(Utc(2026, 6, 21, 12)).x, WithinAbs(23.44, 0.02));
    REQUIRE_THAT(SubSolarPoint(Utc(2026, 12, 21, 12)).x, WithinAbs(-23.44, 0.02));

    // The equinoxes fall at 14:46 UTC in March 2026 and 17:05 UTC in September, so noon on either
    // day is a few hours short of the crossing and the declination has not quite reached zero.
    REQUIRE_THAT(SubSolarPoint(Utc(2026, 3, 20, 12)).x, WithinAbs(0.0, 0.1));
    REQUIRE_THAT(SubSolarPoint(Utc(2026, 9, 23, 12)).x, WithinAbs(0.0, 0.25));

    // And nothing anywhere in the year leaves the band the tilt allows.
    for (int day = 0; day < 366; day++)
    {
        const double declination = SubSolarPoint(Utc(2026, 1, 1, 12) + std::chrono::hours{ 24 * day }).x;
        INFO("day " << day << " of 2026, declination " << declination);
        REQUIRE(std::abs(declination) <= 23.44);
    }
}

TEST_CASE("The sub-solar point runs under the Greenwich meridian at noon", "[space][earth_frame]")
{
    // What ties the Sun to the Earth's rotation, and so the one that fails if either series has
    // the wrong epoch, the wrong rate or the wrong sign: get any of that wrong and the Sun stops
    // being overhead at local noon.
    //
    // Not exactly zero. A sundial runs up to a quarter of an hour early or late against a clock
    // over the course of a year - the equation of time, which comes out of the eccentricity of
    // the orbit and the tilt of the ecliptic, and which is bounded by a little over four degrees
    // of longitude.
    for (int day = 0; day < 366; day++)
    {
        const double longitude = SubSolarPoint(Utc(2026, 1, 1, 12) + std::chrono::hours{ 24 * day }).y;
        INFO("day " << day << " of 2026, longitude " << longitude);
        REQUIRE(std::abs(longitude) < 4.2);
    }

    // And it tracks the clock through the day: fifteen degrees of longitude an hour, westwards.
    const double atNoon = SubSolarPoint(Utc(2026, 8, 22, 12)).y;
    const double atOne = SubSolarPoint(Utc(2026, 8, 22, 13)).y;
    REQUIRE_THAT(WrapDegrees(atOne - atNoon), WithinAbs(-15.0, 0.02));
}

TEST_CASE("The sun is up over the UK in the middle of the afternoon", "[space][earth_frame]")
{
    // The regression this material exists for. The scene used to light the planet from a fixed
    // angle, which put the terminator wherever those two numbers happened to fall - and left the
    // UK in the dark at two o'clock on a summer afternoon.
    constexpr double kLondonLatitude = 51.5;
    constexpr double kLondonLongitude = -0.13;

    // 2pm BST, 22 August 2026.
    REQUIRE_THAT(SolarElevationAt(Utc(2026, 8, 22, 13), kLondonLatitude, kLondonLongitude), WithinAbs(48.5, 0.2));

    // Three in the morning of the same day, which had better not be.
    REQUIRE(SolarElevationAt(Utc(2026, 8, 22, 2), kLondonLatitude, kLondonLongitude) < 0.0);

    // Local noon at the two solstices. These are 90 - latitude, plus and minus the obliquity, so
    // between them they pin that the tilt is applied in the right sense at the right end of the
    // year rather than merely being present.
    REQUIRE_THAT(SolarElevationAt(Utc(2026, 6, 21, 12), kLondonLatitude, kLondonLongitude), WithinAbs(61.9, 0.2));
    REQUIRE_THAT(SolarElevationAt(Utc(2026, 12, 21, 12), kLondonLatitude, kLondonLongitude), WithinAbs(15.1, 0.2));

    // Half the planet is in night at any instant, and at this one it is the half with Sydney in
    // it - which is what would fail if the longitude came out mirrored.
    REQUIRE(SolarElevationAt(Utc(2026, 8, 22, 13), -33.87, 151.21) < 0.0);
}

TEST_CASE("The sun direction shares its axes with the orbits", "[space][earth_frame]")
{
    // At the March equinox the Sun is on the vernal equinox itself, which is ECI +X by
    // definition - and ECIToWorld() puts that on world +Z. This is what pins that the direction
    // the scene is lit from is stated on the same axes as the satellites are drawn on, rather
    // than a quarter turn away from them.
    const glm::dvec3 atEquinox = ECIToWorld(CalculateSunDirectionECI(Utc(2026, 3, 20, 12)));
    REQUIRE_THAT(atEquinox.x, WithinAbs(0.0, 0.01));
    REQUIRE_THAT(atEquinox.y, WithinAbs(0.0, 0.01));
    REQUIRE_THAT(atEquinox.z, WithinAbs(1.0, 0.01));

    // The series rotates the unit vector (cos, sin, 0), so it comes out unit length without a
    // normalize. Callers hand it straight to a shader as a light direction; this is what says
    // they may.
    for (unsigned month = 1; month <= 12; month++)
    {
        INFO("month " << month);
        REQUIRE_THAT(glm::length(CalculateSunDirectionECI(Utc(2026, month, 15, 6))), WithinAbs(1.0, 1e-12));
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

TEST_CASE("ECEF is ECI with the planet's turn taken out", "[space][earth_frame]")
{
    const glm::dvec3 eci(4000.0, -2500.0, 5100.0);

    // The frames coincide when the prime meridian is on the vernal equinox, which is what GMST 0
    // means. Anything else and one of the two rotations has picked up a sign or an offset.
    const glm::dvec3 atZero = ECIToECEF(eci, 0.0);
    REQUIRE_THAT(atZero.x, WithinAbs(eci.x, 1e-9));
    REQUIRE_THAT(atZero.y, WithinAbs(eci.y, 1e-9));
    REQUIRE_THAT(atZero.z, WithinAbs(eci.z, 1e-9));

    const glm::dvec3 roundTripped = ECEFToECI(ECIToECEF(eci, glm::radians(123.0)), glm::radians(123.0));
    REQUIRE_THAT(roundTripped.x, WithinAbs(eci.x, 1e-9));
    REQUIRE_THAT(roundTripped.y, WithinAbs(eci.y, 1e-9));
    REQUIRE_THAT(roundTripped.z, WithinAbs(eci.z, 1e-9));

    // The spin axis is the one direction the rotation leaves alone.
    const glm::dvec3 polar = ECIToECEF(glm::dvec3(0.0, 0.0, 6356.0), glm::radians(77.0));
    REQUIRE_THAT(polar.x, WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(polar.y, WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(polar.z, WithinAbs(6356.0, 1e-9));
}

TEST_CASE("Geodetic coordinates anchor to the WGS84 axes", "[space][earth_frame]")
{
    // The four points where the ellipsoid's normal does pass through the centre, and so the only
    // places the answer can be written down without solving anything. These pin the axis
    // convention: +X at the prime meridian, +Y at 90 east, +Z at the north pole.
    const glm::dvec3 greenwich = GeodeticToECEF(glm::dvec3(0.0, 0.0, 0.0));
    REQUIRE_THAT(greenwich.x, WithinAbs(kEarthSemiMajorAxis, 1e-9));
    REQUIRE_THAT(greenwich.y, WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(greenwich.z, WithinAbs(0.0, 1e-9));

    const glm::dvec3 east = GeodeticToECEF(glm::dvec3(0.0, 90.0, 0.0));
    REQUIRE_THAT(east.x, WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(east.y, WithinAbs(kEarthSemiMajorAxis, 1e-9));

    const glm::dvec3 northPole = GeodeticToECEF(glm::dvec3(90.0, 0.0, 0.0));
    REQUIRE_THAT(northPole.z, WithinAbs(kEarthSemiMinorAxis, 1e-9));

    const glm::dvec3 southPole = GeodeticToECEF(glm::dvec3(-90.0, 0.0, 0.0));
    REQUIRE_THAT(southPole.z, WithinAbs(-kEarthSemiMinorAxis, 1e-9));

    // Solving back out has to reach the poles too, where the usual p/cos(lat) form of the altitude
    // divides by zero.
    const glm::dvec3 recovered = ECEFToGeodetic(northPole);
    REQUIRE_THAT(recovered.x, WithinAbs(90.0, 1e-9));
    REQUIRE_THAT(recovered.z, WithinAbs(0.0, 1e-9));
}

TEST_CASE("Geodetic and ECEF round trip at every altitude we track", "[space][earth_frame]")
{
    // Bowring's seed is exact at the surface and degrades with height, so the altitudes here run
    // from the ground out past geostationary. If the refinement iterations are ever removed, the
    // high-altitude rows are what notices.
    for (double altitude : { 0.0, 420.0, 20200.0, 35786.0 })
    {
        for (double latitude : { -90.0, -66.5, -23.5, 0.0, 12.25, 45.0, 51.6, 89.9 })
        {
            for (double longitude : { -179.0, -74.0, 0.0, 33.5, 151.2 })
            {
                const glm::dvec3 geodetic(latitude, longitude, altitude);
                const glm::dvec3 roundTripped = ECEFToGeodetic(GeodeticToECEF(geodetic));

                INFO("lat=" << latitude << " lon=" << longitude << " alt=" << altitude);
                REQUIRE_THAT(roundTripped.x, WithinAbs(latitude, 1e-9));
                REQUIRE_THAT(roundTripped.z, WithinAbs(altitude, 1e-7));

                // Longitude is undefined at the poles, where every meridian meets.
                if (std::abs(latitude) < 90.0)
                {
                    REQUIRE_THAT(WrapDegrees(roundTripped.y - longitude), WithinAbs(0.0, 1e-9));
                }
            }
        }
    }
}

TEST_CASE("Geodetic latitude differs from geocentric by the flattening", "[space][earth_frame]")
{
    // The reason any of this exists. For a point on the ellipsoid the exact relation is
    // tan(geocentric) = (1 - e^2) * tan(geodetic), and the gap peaks near 45 degrees at about
    // 0.192 degrees - some 21 km of ground, which is what the readout used to be out by.
    for (double geodeticLatitude : { 0.0, 15.0, 30.0, 45.0, 60.0, 75.0, 90.0 })
    {
        const glm::dvec3 ecef = GeodeticToECEF(glm::dvec3(geodeticLatitude, 0.0, 0.0));
        const double expected = glm::degrees(std::atan((1.0 - kEarthEccentricitySq) * std::tan(glm::radians(geodeticLatitude))));

        INFO("geodetic=" << geodeticLatitude);
        REQUIRE_THAT(GeocentricLatitudeOf(ecef), WithinAbs(geodeticLatitude == 90.0 ? 90.0 : expected, 1e-9));

        // And the solve gives back the geodetic latitude we started from, not the geocentric one.
        REQUIRE_THAT(ECEFToGeodetic(ecef).x, WithinAbs(geodeticLatitude, 1e-9));
    }

    // The peak, written out so the magnitude is documented and not just implied.
    const glm::dvec3 at45 = GeodeticToECEF(glm::dvec3(45.0, 0.0, 0.0));
    REQUIRE_THAT(45.0 - GeocentricLatitudeOf(at45), WithinAbs(0.19241, 1e-4));

    // Zero at the equator and the poles, and geocentric always the one nearer the equator.
    REQUIRE_THAT(GeocentricLatitudeOf(GeodeticToECEF(glm::dvec3(0.0, 0.0, 0.0))), WithinAbs(0.0, 1e-12));
    for (double latitude : { 5.0, 45.0, 80.0 })
    {
        REQUIRE(GeocentricLatitudeOf(GeodeticToECEF(glm::dvec3(latitude, 0.0, 0.0))) < latitude);
    }
}

TEST_CASE("Altitude is measured from the ellipsoid, not a mean sphere", "[space][earth_frame]")
{
    // Subtracting a 6371 km mean radius from the distance to the centre is the obvious way to get
    // an altitude and is what this used to do. It is wrong by the difference between the mean
    // sphere and the ellipsoid at that latitude, which swings 21 km between the equator and the
    // poles - twice an orbit, for anything inclined.
    constexpr double kMeanRadius = 6371.0;

    const glm::dvec3 overEquator = GeodeticToECEF(glm::dvec3(0.0, 20.0, 400.0));
    REQUIRE_THAT(ECEFToGeodetic(overEquator).z, WithinAbs(400.0, 1e-7));
    REQUIRE_THAT(glm::length(overEquator) - kMeanRadius - 400.0, WithinAbs(7.137, 1e-3));

    const glm::dvec3 overPole = GeodeticToECEF(glm::dvec3(90.0, 0.0, 400.0));
    REQUIRE_THAT(ECEFToGeodetic(overPole).z, WithinAbs(400.0, 1e-7));
    REQUIRE_THAT(glm::length(overPole) - kMeanRadius - 400.0, WithinAbs(-14.248, 1e-3));
}

TEST_CASE("The surface mapping puts Greenwich on model +X and north at v zero", "[space][earth_frame]")
{
    REQUIRE_THAT(SurfaceLongitudeAt(glm::dvec3(1.0, 0.0, 0.0)), WithinAbs(0.0, 1e-5));
    REQUIRE_THAT(static_cast<double>(SurfaceUVAt(glm::dvec3(0.0, 1.0, 0.0)).y), WithinAbs(0.0, 1e-6));

    // East has to run the way the planet turns. A positive rotation about world +Y carries +Z to
    // +X, so east of Greenwich is towards -Z; get this backwards and the map is mirrored.
    REQUIRE(SurfaceLongitudeAt(glm::dvec3(1.0, 0.0, -1.0)) > 0.0);
}

TEST_CASE("The surface mapping is linear in geodetic latitude", "[space][earth_frame]")
{
    // Equirectangular maps are laid out in geodetic latitude, but the mesh hands this function a
    // direction whose asin is the *reduced* latitude - a third convention, sitting almost exactly
    // midway between geodetic and geocentric. Sampling with the raw asin shifts coastlines about
    // 10.7 km polewards near 45 degrees, so v has to convert.
    for (double reducedLatitude : { -80.0, -45.0, -10.0, 0.0, 22.0, 45.0, 67.5 })
    {
        const glm::dvec3 direction = MeshDirection(reducedLatitude, 30.0);

        INFO("reduced=" << reducedLatitude);
        REQUIRE_THAT(SurfaceLatitudeAt(direction), WithinAbs(ECEFToGeodetic(ModelToECEF(MeshVertex(direction))).x, 2e-5));
    }

    // The magnitude of what the conversion is worth, and its direction: geodetic is further from
    // the equator than the reduced latitude the mesh is parameterised by.
    REQUIRE_THAT(SurfaceLatitudeAt(MeshDirection(45.0, 0.0)) - 45.0, WithinAbs(0.09621, 1e-4));

    // A sphere has no such distinction, and passing equal radii has to collapse back to the plain
    // asin - otherwise this function could not be used for any other planet.
    const glm::vec3 direction(0.3f, 0.5f, -0.2f);
    const float sphereV = DirectionToSurfaceUV(glm::normalize(direction), 1000.0f, 1000.0f).y;
    const float expectedV = 0.5f - std::asin(glm::normalize(direction).y) / glm::pi<float>();
    REQUIRE_THAT(static_cast<double>(sphereV), WithinAbs(static_cast<double>(expectedV), 1e-6));
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
    // Four separate conventions have to agree: the mesh's geometry, its texture mapping, the
    // planet's model matrix, and the swizzle that places satellites. Nothing but this relates them,
    // and when the model matrix disagreed the error was GMST - 90 degrees - up to a third of the way
    // round the planet, drifting 15 degrees an hour, and briefly small enough once a sidereal day to
    // look plausible.
    //
    // Read forwards: take a real vertex off the mesh, ask the texture what ground it shows, rotate
    // it into world space, then ask which satellite would be drawn exactly there. That satellite's
    // reported sub-point must be the ground we started from - and, because the vertex is on the
    // ellipsoid, at zero altitude. The altitude assertion is what pins that the shape the readout
    // solves against is the shape the planet is drawn as.
    for (int hour = 0; hour < 24; hour++)
    {
        const double gmst = glm::radians(hour * 15.0);
        const glm::dmat4 rotation = CalculatePlanetRotation(gmst);

        for (double reducedLatitude : { -67.0, -23.5, 0.0, 38.9, 71.0 })
        {
            for (double azimuth : { -175.0, -90.0, 0.0, 45.0, 128.0 })
            {
                const glm::dvec3 modelDirection = MeshDirection(reducedLatitude, azimuth);
                const glm::dvec3 world(rotation * glm::dvec4(MeshVertex(modelDirection), 1.0));
                const glm::dvec3 reported = ECIToGeodetic(WorldToECI(world), gmst);

                INFO("gmst=" << hour * 15 << "deg reduced=" << reducedLatitude << " az=" << azimuth);
                REQUIRE_THAT(WrapDegrees(reported.y - SurfaceLongitudeAt(modelDirection)), WithinAbs(0.0, 2e-5));

                // Latitude is invariant under a spin about the polar axis. This is what fails if
                // the planet is ever rotated about the wrong axis.
                REQUIRE_THAT(reported.x - SurfaceLatitudeAt(modelDirection), WithinAbs(0.0, 2e-5));

                REQUIRE_THAT(reported.z, WithinAbs(0.0, 1e-6));
            }
        }
    }
}

TEST_CASE("Sub-satellite longitude tracks the planet turning underneath", "[space][earth_frame]")
{
    // A fixed inertial direction should have the ground slide westwards beneath it as the Earth
    // turns east, at very nearly fifteen degrees an hour.
    const glm::dvec3 fixedInertialPosition(7000.0, 0.0, 0.0);

    const double before = ECIToGeodetic(fixedInertialPosition, 0.0).y;
    const double after = ECIToGeodetic(fixedInertialPosition, glm::radians(15.0)).y;

    REQUIRE_THAT(WrapDegrees(after - before), WithinAbs(-15.0, 1e-9));
}
