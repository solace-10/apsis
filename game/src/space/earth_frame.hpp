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

// Sub-satellite point for an ECI position, as (latitude, longitude) in degrees.
//
// The latitude is geocentric, not geodetic: it is the angle at the centre of the Earth rather than
// the angle a plumb line makes with the equatorial plane, and on an oblate planet those differ by
// up to ~0.19 degrees. Tracking sites report geodetic.
glm::dvec2 ECIToLatLon(const glm::dvec3& eciPosition, double gmst);

// Equirectangular texture coordinate for a point on the planet's surface, given the unit direction
// from the centre in model space. u runs east from longitude 180W at 0, v runs south from the north
// pole at 0, matching how the surface maps are laid out.
glm::vec2 DirectionToSurfaceUV(const glm::vec3& direction);

} // namespace WingsOfSteel
