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

## The reference propagator is the yardstick, and it is checked first

`game/tests/reference/sgp4/` holds Vallado's SGP4 unmodified, and everything the suite says about
propagation is measured against it. That is only worth anything if the copy in this tree behaves
like the one he published, so `The vendored SGP4 reference reproduces Vallado's published
verification output` runs all 33 element sets of `SGP4-VER.TLE` over the ranges the file itself
asks for and compares against `tcppver.out`. It agrees to the last digit the file prints —
5e-9 km, 5e-10 km/s — everywhere except the 3.5-year propagation of 20413, which reaches 1.2e-7 km.
The assertions sit an order of magnitude above that, so they measure our build against Vallado's
rather than the printing.

Two details of that comparison are easy to mistake for noise and are not. The row count is asserted
as well as the rows, because it is where the propagation gave up — 33333 stops after five steps with
error 4, and pinning that pins the point at which an element set starts failing. And 33334 has one
row in the file that is stale output from the previous satellite: it fails at its own epoch, so it
has no results of its own, and it is covered by a case about the error code instead.

- Recorded by: `The vendored SGP4 reference reproduces Vallado's published verification output`,
  `A satellite whose orbit decays partway through the range stops there`, `An element set that fails
  at its own epoch produces no positions at all`, `The near-circular case labelled an error attempt
  in fact propagates` (sgp4_tests.cpp).

## What today's propagation costs, as a number

`Two-body propagation of SGP4 mean elements drifts by a known amount` measures
`CalculateCartesianPosition()` against the reference for 06251, a real 377 km-perigee low Earth
orbit: about 13 km wrong at the epoch, and 416 km at worst over the following day.

The bounds are a band rather than a ceiling, deliberately. If the propagation is ever made correct
without this case being revisited it will fail, rather than pass more comfortably and say nothing —
which is the only way a test like this can act as a before-and-after for the SGP4 work.

## The compute path is reachable from the suite

`gpu/compute_harness.{hpp,cpp}` brings up a Dawn device with no window, compiles a `.wgsl` file
through the engine's own preprocessor, dispatches it and blocks until the results are back. It
restates `SGP4ComputePass`'s sequencing — the pass spreads its readback over several frames because
`RenderSystem::Update()` owns the encoder — but deliberately not its data: the input and output
structs come from `render/sgp4_compute_pass.hpp`, so a test cannot disagree with production about
the layout it is checking.

`The compute shader propagates as accurately as f32 allows` dispatches `sgp4.wgsl` itself, from the
tree the game ships from. It covers the struct layout, the binding indices, the guard on the tail
of a rounded-up dispatch and the readback, as the echo case it replaced did — and now the
propagation as well.

The harness skips rather than fails where there is no adapter, and it is native-only: the readback
blocks on `ProcessEvents`, and there is nothing on the web to block with.

## BSTAR is the only drag term carried, and it is required

A GP record carries three drag terms. Only BSTAR is selected, served or stored: SGP4 consumes it
when initialising an element set and again at every step, while `MEAN_MOTION_DOT` and
`MEAN_MOTION_DDOT` are the Taylor coefficients of the SGP model SGP4 replaced — which is why the
TLE names them pre-divided by 2 and 6 — and Vallado's `sgp4init` takes them only to store them,
never reading them again. So passing zero for both when an element set is initialised is correct
rather than a placeholder. They remain in the database, where they cost nothing and keep the
ingested record whole.

BSTAR is required, exactly like the six mean elements beside it. The OMM standard marks it
conditional rather than mandatory — CCSDS 502.0-B-3 table 4-3, where `C` means mandatory once a
stated condition holds — but that condition is `MEAN_ELEMENT_THEORY = SGP/SGP4`, which is what
every record in this catalogue is, and Space-Track populates it for all of them. A row without one
is not a producer exercising the latitude the standard gives it; it is a fault.

Nothing along the path softens that, deliberately. `get_all_objects` selects the column raw and
decodes it into a non-optional `f64`, so a NULL fails the query rather than arriving as a number,
and `OrbitalElementsComponent::Deserialize` asks for the key with no default, so an absent one is
an error like any other missing element.

Reading an absent drag term as zero is the dangerous alternative rather than the safe one. Zero is
a value SGP4 accepts and propagates perfectly happily — it is what an unmodelled drag term is — so
a broken deployment would present as a whole catalogue sitting slightly in the wrong place, with
nothing anywhere saying why. An error at the point the field goes missing is louder and cheaper.

- Recorded by: `An element set carries the drag term SGP4 needs`
  (orbital_elements_component_tests.cpp).

## Initialisation is ours, and it is checked coefficient by coefficient

SGP4 divides into a part that depends only on the element set and a part that depends on time.
`game/src/space/sgp4.{hpp,cpp}` is the first of those: one element set in, about thirty
coefficients out, run once per object rather than once per object per frame. The step is what will
eventually run in the shader, and keeping the initialisation in double on the CPU is what stops
WGSL's lack of `f64` mattering more than it has to.

The implementation is our own rather than Vallado's file moved across. `reference/sgp4/README.md`
firewalls that copy to the `tests` target because its licence position is unresolved, and a
rewrite is worth exactly what its comparison against the original is worth — so the comparison is
every coefficient, over every near-Earth element set in the verification file, at **exact
equality**. The expressions are written in the reference's own operation order for that reason. A
tolerance here would be somewhere for a rearrangement to hide, and there is nothing to hide: the
two agree bit for bit.

The partition between the branches is read from `satrec.method` rather than from a 225-minute
threshold restated in the test, which would only check the test against itself. It comes out at
9 near-Earth cases and 24 deep-space ones.

Deep space initialises to nothing at all. The reference computes the near-Earth coefficients for
those orbits too and then adds the lunar-solar and resonance terms on top, so stopping at the
partition and returning a zeroed block is the difference between an obviously unusable result and
one that looks usable and is half missing. The partition case asserts those zeroes rather than
merely tolerating them.

Two things the tests established rather than assumed:

- **The epoch offset is 7306 days, not the 7305 the reference subtracts.** Both appear in the same
  arithmetic and they are a day apart: SGP4 initialises from 1950 January 0.0, while `initl` counts
  back to 1970 January 0.0 — the last day of 1969 — on its way to the sidereal time at epoch. The
  conversion case caught this, off by exactly one day.
- **The gravity constants are WGS72**, the model TLEs are fitted with; propagating an element set
  with any other set of constants uses constants its own fit did not. That leaves three Earth radii
  in the tree: `kSGP4EarthRadius` at 6378.135, `kEarthSemiMajorAxis` at WGS84's 6378.137, and
  `kMu = 398600.4418` in `orbit_propagation_system.cpp`, which is neither. The first two are
  deliberate and answer different questions — where the propagator's arithmetic is defined, and
  where the ground is. The third feeds the two-body path and the vis-viva velocity and is simply
  unexamined; it leaves with the propagation it belongs to.

The one place a float sits in the way is `MakeSGP4Elements()`. `OrbitalElementsComponent` stores
floats, so about seven significant digits reach an algorithm written in double — dominated by mean
motion, worth roughly twenty metres of along-track error after a day, two orders of magnitude
inside SGP4's own accuracy and far inside what an f32 step will contribute. That case is held to
float precision rather than to the arithmetic's exactness, and says so.

Nothing consumes the coefficients yet, but they are derived once and then held on the entity.
`SGP4Component` is added beside `OrbitalElementsComponent` in `Sector::InitializeSpaceObjects`,
which is the last moment its inputs can change: everything afterwards only decides which objects
are visible. `NotifyGroupFiltersChanged` clears four components and leaves this one alone, so a
filter toggle costs nothing here — and because entt owns the lifetime, the handle-recycling hazard
that rules out keying a cache on an entity never arises.

Deep-space objects carry the component too, holding the zeroed block, so an object that is not
being propagated says as much on itself rather than by being missing from something.

- Recorded by: `Our initialisation reproduces the reference's near-Earth coefficients`, `The
  near-Earth and deep-space partition matches the reference`, `An element set converts into the
  units SGP4 initialises from` (sgp4_init_tests.cpp).

## The step is written twice, and the first one is exact

`SGP4Step()` propagates an initialised element set to a time in minutes from its epoch. It is the
other half of the algorithm, and unlike the initialisation it does eventually have to run in f32 on
the GPU — which is exactly why it exists in double here first.

A WGSL step compared against Vallado's would be measuring two things at once: whether the algorithm
was transcribed correctly, and what f32 costs. A failure would not say which. This version settles
the first question separately, and settles it the strongest way available — **exact equality**,
position and velocity, over every near-Earth element set in the verification file at every time it
asks to be propagated to. 160 positions across 9 cases, bit for bit. Whatever the shader turns out
to differ by is then precision and nothing else.

Four of those nine give up before the end of the range they ask for — 22312, 28350, 28872 and
29141 — and the agreement covers that too: the same failure, at the same step. Where a propagation
stops being valid is a separate thing to be right about from where the object is, and only one of
the two is visible in a position.

Two asymmetries the cases pin rather than tidy away:

- **A decayed orbit still has a position.** The reference gives up before writing one for the
  eccentricity, mean motion and semi-latus-rectum failures, but the decay check is made *from* the
  position, so that one is written first and then rejected. `SGP4Position` does the same, which is
  why the comparison runs one step past the last row `tcppver.out` prints for two of these cases —
  160 positions against 158 published.
- **Deep space is refused rather than attempted.** `SGP4Initialise()` zeroes everything for those
  element sets, so `no_unkozai` is zero and an unguarded step's first act would be to divide by it.
  `SGP4Step()` checks the branch before any arithmetic and returns `DeepSpaceNotSupported`. That is
  what makes a zeroed block safe for `SGP4Component` to carry on every geostationary object in the
  catalogue.

One thing to settle before the shader rather than discover during it: the Kepler solve is
Newton-Raphson capped at ten passes with each correction clamped to 0.95, and its convergence
tolerance is 1e-12. That is below f32 epsilon, so the WGSL version cannot reach it and will always
run all ten passes. Bounded and correct, but it should be a decision.

- Recorded by: `Our step reproduces the reference propagator over the near-Earth cases`, `A decaying
  orbit stops where the reference stops`, `A deep-space element set cannot be stepped`
  (sgp4_step_tests.cpp).

## What f32 costs, measured

`sgp4.wgsl` propagates. It is a transcription of `SGP4Step()` and it is compared against that
rather than against Vallado, which is the whole reason the step was written twice: both CPU halves
agree with the reference exactly, so every difference the shader shows is precision and nothing
else.

**295 metres, and 2.3e-4 km/s.** Worst across 162 samples — every near-Earth element set in the
verification file at every time it asks to be propagated to. That is the answer to the question
this design has been resting on from the start, and it is a comfortable one: SGP4's own accuracy is
on the order of a kilometre at epoch and grows by kilometres a day, so f32 in the step costs
considerably less than the model being evaluated.

Where the error is *not*, both established rather than assumed:

- **Not the coefficient narrowing.** Running the double step with its coefficients round-tripped
  through f32 accounts for 32 m of the 295. The other ninety per cent is arithmetic inside the
  shader.
- **Not accumulation over elapsed time.** 29141 is 200 m out at t = 180 minutes while 22312 is
  25 m out at t = 274. The error tracks the orbit rather than the time, which is what argues
  against trigonometric argument reduction being the dominant term — that would grow with t.

So the lever, if this ever needs improving, is the step's own f32 arithmetic; finding which part of
it is a matter of attributing further rather than guessing.

Two things worth knowing before going looking:

- The Kepler solve's convergence tolerance is 1e-12, below f32 epsilon, so the shader cannot use
  the CPU's early exit and always runs all ten passes. It converges long before that, so what this
  costs is iterations rather than accuracy.
- **The measurement deliberately excludes the time going in.** Both sides are given the same
  f32-representable `tsince`, so this is the cost of the arithmetic alone. At ten days from epoch an
  f32 `tsince` is worth several hundred metres of along-track error by itself — comparable to
  everything measured here — which makes how time reaches the GPU a real decision rather than a
  detail of the upload.

- Recorded by: `The compute shader propagates as accurately as f32 allows` (sgp4_shader_tests.cpp).

## Not covered yet

Known gaps, in rough order of how much they'd be worth:

- **Nothing tells the GPU what time it is.** `UpdateRoster` uploads the coefficients and a `tsince`
  of zero, so every object is propagated to its own epoch and sits there, and
  `ApplyPropagatedPositions` still writes numbers nobody should believe. The coefficients only need
  re-uploading when the roster changes but the time changes every frame, so the two probably want
  separate buffers — which is a third bind group entry, and `ComputeHarness::DispatchRaw()`
  hard-codes two. Whatever carries it has to answer the f32 `tsince` hazard above.
- **The velocity comes back and is thrown away.** The shader computes it and `SGP4StepOutput`
  carries it, but `PropagationResults` keeps only positions and `UpdateOrbitalState` still derives
  speed from vis-viva on a two-body semi-major axis. The real one is already paid for.
- **Deep space.** `SGP4Initialise()` reports `SGP4Method::DeepSpace` and stops. Those objects stay
  on the two-body path, which is where they already were, but they are not a rounding error in this
  catalogue: `celestrak.py` curates `geo`, `gnss` and `gps-ops`, and the served query is unfiltered.
  SDP4's step also carries integrator state between calls, so it does not fit the split the
  near-Earth path was built around and needs its own answer on the GPU.
- **Nothing pins the render shaders.** `planet.wgsl` consumes the model matrix and must apply it to
  both position and normal; the atmosphere and wireframe pipelines deliberately do not. Unlike the
  compute path, which the harness can now drive, these run inside a render pass against a swapchain
  and the check is visual. `sun.wgsl` is in the same position and adds a second thing to look at:
  the disc is built from the same light direction the terminator is, so a screenshot showing the Sun
  off to one side of the daylight would mean the billboard's camera basis is wrong rather than the
  ephemeris.
- **`CalculateGMST()` uses UTC where the series wants UT1.** Unix time ignores leap seconds, so the
  argument can be up to a second out — under a hundredth of a degree, and so far below
  anything else in this file. Recorded here so it is a known approximation rather than a latent surprise.
