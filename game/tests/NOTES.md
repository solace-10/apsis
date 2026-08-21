# Test notes

Behaviour the tests pin down but which may not be what we actually want, and known gaps. Each entry
names the test that records the current behaviour, so changing the code means changing a test on
purpose rather than discovering a surprise.

## Latitude is geocentric, not geodetic

`ECIToLatLon()` returns the angle at the centre of the Earth, `atan2(z, r_xy)`. Every tracking site
reports geodetic latitude, which is the angle a plumb line makes with the equatorial plane, and on
an oblate planet the two differ by up to ~0.19 degrees — around 21 km near 45 degrees.

The planet is *drawn* as a WGS84 ellipsoid (`Sector::Initialize()` sets a distinct semi-minor
radius), so the readout and the mesh disagree about what latitude means. Only the readout is
affected: the 3D position is computed in Cartesian ECI throughout and never round-trips through a
latitude.

- Recorded by: `A satellite is drawn over the ground it is reported to be over`
  (earth_frame_tests.cpp), which asserts the latitude is invariant under the planet's spin — true of
  both conventions, so it pins the axis of rotation without taking a side on this.

## The quarter turn in the planet's rotation is a convention, not physics

`CalculatePlanetRotation()` returns `R_y(gmst - 90deg)`. The 90 degrees is not an epoch or a
calibration: it is the composition of two independent conventions — the mesh carries longitude 0 on
model +X (`DirectionToSurfaceUV()`), and `ECIToWorld()` puts world +X on right ascension 90 degrees.
Change either and the quarter turn has to change with it.

This is what was wrong when satellites appeared over the wrong ground: the planet was never rotated
at all, which is not a stationary Earth so much as one frozen at the single orientation it should
hold when GMST is 90 degrees.

- Recorded by: `An unrotated planet sits at a sidereal time of 90 degrees` (earth_frame_tests.cpp).

## Not covered yet

Known gaps, in rough order of how much they'd be worth:

- **The orbital propagation itself.** `OrbitSimulationSystem::CalculateCartesianPosition()` treats
  SGP4 mean elements as classical Keplerian and propagates two-body, so the secular J2 drifts are
  missing (~-5 deg/day of nodal regression for an ISS-like orbit). This is known and deferred — see
  `[5]` in TODO.txt — but it is now *reachable*: the suite links game_lib, so a test can construct
  an `OrbitalElementsComponent` and pin positions against a reference propagator. Worth doing
  before the SGP4 work lands, so the change has something to move against.
- **`CalculateCartesianPosition()` reads the clock per satellite.** It calls
  `system_clock::now()` inside itself, once per object, so satellites in a single frame are
  propagated at fractionally different instants and at a different instant from the GMST the
  planet is oriented with. The discrepancy is microseconds and therefore sub-millimetre, but it is
  the same class of mistake as the GMST bug, and taking the instant as a parameter — the way
  `CalculateGMST()` now does — would both fix it and make the propagation testable deterministically.
- **Nothing pins the shader side.** `planet.wgsl` consumes the model matrix and must apply it to
  both position and normal; the atmosphere and wireframe pipelines deliberately do not. That is
  WGSL running on a GPU, so the suite cannot see it. The check is visual.
- **`CalculateGMST()` uses UTC where the series wants UT1.** Unix time ignores leap seconds, so the
  argument can be up to a second out — under a hundredth of a degree, and far below the geodetic
  discrepancy above. Recorded here so it is a known approximation rather than a latent surprise.
