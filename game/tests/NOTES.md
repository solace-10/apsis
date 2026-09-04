# Test notes

Behaviour the tests pin down but which may not be what we actually want, and known gaps. Each entry
names the test that records the current behaviour, so changing the code means changing a test on
purpose rather than discovering a surprise.

## Three latitudes, and the one the code speaks

An oblate planet has three different answers to "how far north is this point?", and all three are
reachable from code that is already in this repository:

- **Geodetic** — the angle the ellipsoid's surface normal makes with the equatorial plane. What
  WGS84, GPS and every tracking site report, and what equirectangular surface maps are laid out in.
- **Geocentric** — the angle subtended at the centre, `atan2(z, r_xy)`. What falls out of Cartesian
  coordinates for free, and so the one reached for by accident. Up to 0.192 degrees from geodetic —
  around 21 km of ground — near 45 degrees.
- **Reduced** — `asin(dir.y)` for the direction the mesh generator projects a cube vertex onto,
  because the vertex it places is `(dir.x*a, dir.y*b, dir.z*a)`. Sits almost exactly midway between
  the other two; sampling the surface map with it shifts coastlines about 10.7 km polewards.

Everything the user sees is geodetic. `ECIToGeodetic()` solves for it with Bowring's closed form
plus two turns of the exact fixed point, and `DirectionToSurfaceUV()` converts from reduced before
computing v. Geocentric latitude appears only inside the tests, as the thing being measured against.

The same solve returns altitude above the ellipsoid rather than above a mean sphere. A 6371 km mean
radius is off by +7.1 km at the equator and -14.2 km at the poles, so an inclined orbit's reported
height would swing 21 km twice an orbit.

- Recorded by: `Geodetic latitude differs from geocentric by the flattening`, `The surface mapping is
  linear in geodetic latitude`, `Altitude is measured from the ellipsoid, not a mean sphere`, and
  `Geodetic and ECEF round trip at every altitude we track` (earth_frame_tests.cpp).

## No second, geocentric sub-satellite point

`OrbitalStateComponent` holds one latitude, not one per convention. Longitude is identical either
way, so a second pair would add a field obliged to equal its neighbour with nothing relating them —
the same shape as the bug in `[1]`. A ground track wants a position rather than a latitude anyway:
`GeodeticToECEF(lat, lon, 0)` is that point, and cannot be read in the wrong convention.

## The quarter turn in the planet's rotation is a convention, not physics

`CalculatePlanetRotation()` returns `R_y(gmst - 90deg)`. The 90 degrees is not an epoch or a
calibration: it is the composition of two independent conventions — the mesh carries longitude 0 on
model +X (`DirectionToSurfaceUV()`), and `ECIToWorld()` puts world +X on right ascension 90 degrees.
Change either and the quarter turn has to change with it.

This is what was wrong when satellites appeared over the wrong ground: the planet was never rotated
at all, which is not a stationary Earth so much as one frozen at the single orientation it should
hold when GMST is 90 degrees.

- Recorded by: `An unrotated planet sits at a sidereal time of 90 degrees` (earth_frame_tests.cpp).

## The sun direction is stated in the mean equinox of date

`CalculateSunDirectionECI()` returns the Sun referred to the mean equinox of date, not to J2000.
That is deliberate, and it is the same choice `ECIToECEF()` already makes: GMST is measured from
the equinox of date, so pairing it with a J2000 direction would smear the sub-solar point by the
accumulated precession — about 0.36 degrees by 2026, and growing 50 arcseconds a year.

The series itself is the Astronomical Almanac's low precision one, good to roughly 0.01 degrees
between 1950 and 2050. Two approximations sit under that and are both far smaller than it: the
argument is UT where the series wants TT, worth 0.0008 degrees, and the ecliptic latitude is taken
as zero rather than the arcsecond or so of it the Moon and planets induce.

- Recorded by: `The sun's declination follows the obliquity through the year`, `The sub-solar point
  runs under the Greenwich meridian at noon`, `The sun is up over the UK in the middle of the
  afternoon`, `The sun direction shares its axes with the orbits` (earth_frame_tests.cpp).

The last of these is the one worth keeping honest. The direction is handed to the renderer through
`ECIToWorld()` and ends up as `GlobalUniforms::directionalLightDirection`, from which both the
planet's terminator and the sun disc are drawn — so a quarter turn lost between the two frames
would show up as the Sun visibly not being where the daylight is.

## Not covered yet

Known gaps, in rough order of how much they'd be worth:

- **The orbital propagation itself.** `OrbitPropagationSystem::CalculateCartesianPosition()` treats
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
  WGSL running on a GPU, so the suite cannot see it. The check is visual. `sun.wgsl` is in the same
  position and adds a second thing to look at: the disc is built from the same light direction the
  terminator is, so a screenshot showing the Sun off to one side of the daylight would mean the
  billboard's camera basis is wrong rather than the ephemeris.
- **`CalculateGMST()` uses UTC where the series wants UT1.** Unix time ignores leap seconds, so the
  argument can be up to a second out — under a hundredth of a degree, and so far below
  anything else in this file. Recorded here so it is a known approximation rather than a latent surprise.
