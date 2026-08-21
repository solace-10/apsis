#pragma once

#include <chrono>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace WingsOfSteel
{

// The relationship between the inertial frame the orbits are computed in, the world space they are
// drawn in, and the texture that shows the ground underneath them.
//
// These are deliberately free functions over plain glm types with no engine behind them, so that
// the one thing that has to agree - where a satellite is drawn versus where it is reported to be -
// can be exercised without a device, a scene or a clock. Three separate pieces of code have to
// share one convention for that to come out right, and nothing but a test relates them.
// See game/tests/space/earth_frame_tests.cpp.

// The WGS84 reference ellipsoid, in kilometres. Sector::Initialize() builds the planet mesh to the
// same dimensions: if one of them is ever changed the other has to follow, or the ground the mesh
// draws stops being the ground the readout reports.
inline constexpr double kEarthSemiMajorAxis = 6378.137;
inline constexpr double kEarthFlattening = 1.0 / 298.257223563;
inline constexpr double kEarthSemiMinorAxis = kEarthSemiMajorAxis * (1.0 - kEarthFlattening);
inline constexpr double kEarthEccentricitySq = kEarthFlattening * (2.0 - kEarthFlattening);

// Greenwich Mean Sidereal Time, in radians, for a given instant.
//
// Takes the instant rather than reading the clock so that a whole frame resolves against one Earth
// orientation, and so that the result can be checked against a known epoch.
double CalculateGMST(std::chrono::system_clock::time_point when);

// The planet's model matrix: what turns a mesh built about the origin so that its prime meridian
// sits at the given sidereal time.
//
// The quarter turn is not decoration. The mesh carries geographic longitude 0 on model +X, while
// ECIToWorld() puts world +X on right ascension 90 degrees, so an unrotated planet is frozen at the
// orientation it should only hold when GMST is 90 degrees.
glm::dmat4 CalculatePlanetRotation(double gmst);

// ECI (x towards the vernal equinox, z along the spin axis) to the world space the scene is drawn
// in, which is y-up. Purely an axis relabelling - no rotation, no scale.
glm::dvec3 ECIToWorld(const glm::dvec3& eciPosition);
glm::dvec3 WorldToECI(const glm::dvec3& worldPosition);

// ECEF is Earth-Centered, Earth-Fixed: the origin is the centre of the Earth, +X passes through the
// equator at the prime meridian, +Y through the equator at 90 degrees east, and +Z runs along the
// spin axis towards the north pole. Unlike ECI it turns with the planet, so a point on the ground
// keeps the same coordinates all day; the two frames differ by a single rotation about the spin
// axis through GMST.
//
// Rotating by GMST is the correct pairing for TEME, which is the frame the OMM mean elements are
// actually expressed in - see [5] in TODO.txt for why the positions are labelled ECI regardless.
glm::dvec3 ECIToECEF(const glm::dvec3& eciPosition, double gmst);
glm::dvec3 ECEFToECI(const glm::dvec3& ecefPosition, double gmst);

// ECEF <-> geodetic, where geodetic is packed as (latitude degrees, longitude degrees, altitude km)
// and altitude is measured from the WGS84 ellipsoid, not from a mean sphere.
//
// Geodetic latitude is the angle the ellipsoid's surface normal makes with the equatorial plane,
// which is what WGS84, GPS receivers and every tracking site report. It is NOT the geocentric
// latitude atan2(z, r_xy), the angle subtended at the centre of the Earth: the normal misses the
// centre everywhere except the equator and the poles, and the two differ by up to 0.192 degrees -
// around 21 km of ground - near 45 degrees.
glm::dvec3 ECEFToGeodetic(const glm::dvec3& ecefPosition);
glm::dvec3 GeodeticToECEF(const glm::dvec3& geodetic);

// Sub-satellite point for an ECI position, as (latitude degrees, longitude degrees, altitude km).
// Composes ECIToECEF() with ECEFToGeodetic(); latitude and altitude come out of the same solve, so
// they cannot end up describing different ellipsoids.
glm::dvec3 ECIToGeodetic(const glm::dvec3& eciPosition, double gmst);

// Equirectangular texture coordinate for a point on the planet's surface, given the unit direction
// from the centre in model space. u runs east from longitude 180W at 0, v runs south from the north
// pole at 0, matching how the surface maps are laid out.
//
// The radii are the ones the mesh was built to, and they matter: the mesh generator places a vertex
// at (dir.x*a, dir.y*b, dir.z*a), so asin(dir.y) is the *reduced* latitude of that vertex - a third
// convention, sitting almost exactly halfway between geodetic and geocentric. Surface maps are laid
// out linearly in geodetic latitude, so v has to convert. Pass equal radii for a true sphere, where
// all three latitudes coincide and the conversion is the identity.
glm::vec2 DirectionToSurfaceUV(const glm::vec3& direction, float semiMajorRadius, float semiMinorRadius);

} // namespace WingsOfSteel
