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

## One check ours makes that the reference does not

`SGP4Step()` rejects a negative `tempa` as `SGP4Error::DragModelDiverged`, and `sgp4.wgsl` mirrors
it. This is the only place the propagator deliberately departs from Vallado, so it is worth being
able to say what it costs and what it buys — the enum's own comment covers why the reference does
not make the check, which is not the same as it having been missed.

It costs nothing against the verification file. Across all nine near-earth cases, over each one's
full published range, the minimum `tempa` is 0.9514 and the minimum `am` is 0.9956 — so neither
this check nor the re-enabled `am < 0.95` fires on any verification case, and `Our step reproduces
the reference propagator over the near-Earth cases` still pins our error codes to the reference's
exactly. That also means neither is exercised by the sweep, which is why they have a case of their
own.

What it buys is visible by taking 29141 past the 440 minutes the file asks for. Its drag polynomial
crosses zero at 1392 minutes, where the reference happens to object for its own reasons (error 4,
the semi-latus rectum). By 2784 minutes — still under two days from epoch — it does not object at
all: it returns success, a radius of 1.09 million km and a velocity of 0.6 km/s. The existing
checks are all downstream of `am = a0*tempa²`, which recovers as the divergence worsens; `tempa`
does not.

- Recorded by: `A diverged drag model is rejected where the reference reports success`
  (sgp4_step_tests.cpp), and the 29141 sample in `The compute shader propagates as accurately as
  f32 allows` (sgp4_shader_tests.cpp), which is the only thing covering the shader's copy.

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
every coefficient, over every element set in the verification file, at **exact equality**. The expressions are written in the reference's own operation order for that reason. A
tolerance here would be somewhere for a rearrangement to hide, and there is nothing to hide: the
two agree bit for bit.

The partition between the branches is read from `satrec.method` rather than from a 225-minute
threshold restated in the test, which would only check the test against itself. It comes out at
9 near-Earth cases and 24 deep-space ones.

Deep space gets the near-Earth coefficients as well, and an `SDP4Terms` block on top — which is
what the reference does, and what SDP4 steps with. The comparison covers that block field for
field at the same exact equality: the fifty-odd coefficients `dscom` and `dsinit` produce, over all
24 deep-space cases, including both resonances and both arms of the eccentricity fits inside the
half-day one.

**Five coefficients are compared only on the near-Earth cases, and the reason is a trap.**
`aycof`, `xlcof`, `con41`, `x1mth2` and `x7thm1` are all functions of the inclination, and the
lunar-solar periodics move the inclination — so the deep-space *step* works them out again from the
perturbed value. The reference does that by writing them back into its element record, and its
`sgp4init` ends by stepping to `tsince = 0`. So for a deep-space `satrec` those five fields hold
what its **step** produced at the epoch, not what its **initialisation** produced; comparing ours
against them compares two different quantities that share a name. Initialisation's own values are
not observable in the reference at all. Nothing is lost by stopping there: ours are locals inside
the step rather than fields, so a wrong recomputation would move every deep-space position, and the
step case compares those bit for bit over every published row.

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

Deep-space objects carry the component too, holding both halves of the block, so which algorithm
propagates an object is readable off the object.

- Recorded by: `Our initialisation reproduces the reference's coefficients`, `The near-Earth and
  deep-space partition matches the reference`, `An element set converts into the units SGP4
  initialises from`, `A geostationary element set reaches the deep-space branch through the
  component` (sgp4_init_tests.cpp).

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
- **One case never yields a position at all.** 33334 fails at its own epoch, so the single row
  `tcppver.out` prints for it is never reached — and that row is stale output from the satellite
  before it rather than a position for 33334, as the reference-propagator section records.

Which is why the count comes out at **669 positions against 667 published rows**: three cases end
on a decayed orbit and contribute one comparison each past their last published row (28872, 29141,
and 20413 three and a half years into its long run), while 33334 contributes none.

- Recorded by: `Our step reproduces the reference propagator`, `A decaying orbit stops where the
  reference stops`, `A diverged drag model is rejected where the reference reports success`, `A
  deep-space step depends on nothing but its arguments` (sgp4_step_tests.cpp).

## Deep space keeps no state, which is the whole reason it can be stepped anywhere

SDP4 adds two things to SGP4: the periodic pull of the Sun and Moon, which every deep-space orbit
gets, and a resonance with the Earth's own tesseral harmonics, which only a few are in. Twelve of
the 24 deep-space verification cases are resonant — seven synchronous (one turn a day) and five
half-day (two turns a day on an eccentric orbit) — so both arms are covered, and the other twelve
cover the path where the resonance terms are all zero.

The reason this went unimplemented for so long is that it looks like it cannot fit the
CPU-initialise / GPU-step split the near-Earth path was built around. Two things looked like state:

- **The five coefficients the step recomputes.** `aycof`, `xlcof`, `con41`, `x1mth2` and `x7thm1`
  are functions of the inclination, which the periodics move, so the deep-space step needs them
  again from the perturbed value. The reference writes them back into the element record, which
  would make a step mutate its own input. They are locals in `SGP4Step()` instead, seeded from the
  block and overwritten on the branch — the same arithmetic, and nothing written back.
- **The resonance integrator's position.** The reference carries `atime`, `xli` and `xni` between
  calls, resuming the integration from wherever the last one stopped. That really would be state.
  It is a cache: the integration restarts from `atime = 0` whenever the time changes sign or moves
  nearer the epoch, and otherwise walks fixed 720-minute steps towards the time asked for, so the
  states reachable from zero are a fixed sequence and stopping at any one of them is the same
  arithmetic in the same order however you arrived. Restarting every call is bit-identical.

**The exact comparison is what establishes the second claim, not an argument.** The reference is
used the way it is meant to be — one element record walked forward through its range, its
integrator warm — while ours is handed a `const` coefficient block and restarts from the epoch at
every single time. If those were not the same arithmetic the twelve resonant cases would disagree,
and they do not, over every published row. `A deep-space step depends on nothing but its arguments`
then asks 25954 for the same times out of order, backwards and twice over, which is the ordering a
stateful integrator would get wrong and a forward sweep would not.

What it costs is `|t| / 720` iterations for a resonant orbit and nothing for the rest: three at the
catalogue's median object age and five at its ninetieth percentile, by the numbers in TODO item 10.

Two smaller things the tests pin rather than assume:

- **`dpper` at initialisation does nothing, so there is no equivalent of it here.** The reference
  calls it from `sgp4init` with an `init` flag that skips every write it makes. What it would have
  subtracted is `peo`, `pgho`, `pho`, `pinco` and `plo`, which `dscom` sets to zero and nothing
  else ever writes — so they are identically zero, and `SDP4Terms` does not carry them. The
  initialisation case asserts the reference's own five are zero rather than taking that on trust.
- **`gsto` is not `CalculateGMST()`.** Both answer "where is Greenwich pointing", but
  `earth_frame.hpp`'s is the two-term form and this is the full IAU-82 series in Julian centuries.
  Two terms is right for turning the Earth under the camera, where a hundredth of a degree is
  invisible; `gsto` is the phase a resonance is measured against and feeds `xlamo`, which the
  integrator then carries for months. They are meant to differ, and unifying them would break the
  exact comparison — which is the note's whole purpose, because the duplication otherwise looks
  like something to tidy.

## What f32 costs, measured

`sgp4.wgsl` propagates. It is a transcription of `SGP4Step()` and it is compared against that
rather than against Vallado, which is the whole reason the step was written twice: both CPU halves
agree with the reference exactly, so every difference the shader shows is precision and nothing
else.

**295 metres, and 2.3e-4 km/s.** Worst across 160 samples — every near-Earth element set in the
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

- The Kepler solve exits on `kKeplerTolerance` = 1e-6 rather than the CPU's 1e-12, which f32 cannot
  reach — below it the correction is smaller than `eo1`'s own ULP and the loop stalls rather than
  converges. Measured over the near-Earth cases, 1e-6 is crossed on the second pass for 126 of 160
  samples and the third for the rest, against ten passes before the early exit existed. It cost
  nothing measurable: the worst position error is the same to every digit either way.
- **The measurement deliberately excludes the time going in.** Both sides are given the same
  f32-representable `tsince`, so this is the cost of the arithmetic alone. At ten days from epoch an
  f32 `tsince` is worth several hundred metres of along-track error by itself — comparable to
  everything measured here — which makes how time reaches the GPU a real decision rather than a
  detail of the upload.

### Deep space costs more, and eccentricity is why

The same comparison over the deep-space branch, across 509 samples, needs three numbers rather than
one because the distribution has a long tail:

| | samples | median | p90 | worst |
|---|---|---|---|---|
| near Earth | 160 | — | — | **295 m** |
| deep space, within catalogue reach | 439 | 57 m | 557 m | **56.0 km** |
| deep space, beyond it | 70 | — | — | **79.2 km** |

The bulk of deep space is *better* than near Earth — 57 m median, and only 24 of 439 samples exceed
a kilometre. The tail is two element sets and one property:

- **It is not elapsed time.** The worst in-reach sample is 33333 at t = **20 minutes**, twenty
  minutes from its own epoch. The eleven behind it are all 23333.
- **It is eccentricity.** Those two are e = 0.995 and e = 0.973 — all but parabolic, where position
  is violently sensitive to the eccentric anomaly and f32 has nothing left to give. 23333's own
  comment in the verification file says Kepler fails past about 200 minutes. Nothing shaped like
  that survives in an Earth-orbit catalogue in any number.
- **It is not the resonance integrator**, which was the thing to be suspicious of. Its accumulation
  is real but small next to this.

"Within reach" is |t| ≤ 33 days, which is as far from its epoch as an element set in this catalogue
can be: `spacetrack.py` admits nothing over 30 days old and `clear_stale_objects()` trims what stops
being refreshed after 3. The one case beyond it is 20413's second entry, propagated three and a half
years out to exercise Lyddane's choice; f32 error grows with t, so its 79 km is by construction.

Because the worst case is set by an orbit nothing here will ever hold, the test pins **p90 as well
as the maximum**. A wrong term in the transcription moves every sample; an awkward orbit moves one.

### The resonance integrator is capped, on both sides

`dspace` walks 720-minute steps from the epoch, so it needs |t|/720 of them, and the reference lets
that run unbounded. `sgp4.wgsl` cannot — and a bound the shader kept that `SGP4Step()` did not would
stop the two being comparable, which is what this whole section rests on. So `kResonanceMaxSteps`
= 72 lives in both, and exceeding it raises `SGP4Error::ResonanceStepLimitExceeded`.

72 comes from the same 33-day reach: 47,520 minutes is 66 steps, and 72 leaves a margin. It changes
nothing the tests compare — across every case in the file at every published time the integrator's
worst is **14** steps and its mean is 2.3 — so the limit is only reachable by an element set the
ingestion should already have dropped. Refusing one beats the alternative, which is not a smaller
error but a meaningless one: the unintegrated remainder of `t` goes into a quadratic.

- Recorded by: `The compute shader propagates as accurately as f32 allows` (sgp4_shader_tests.cpp)
  and `A resonance too far from its epoch is refused where the reference reports success`
  (sgp4_step_tests.cpp), which reaches a limit the sweep cannot — the same shape as the drag case.

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
- **Deep space runs on the CPU while the shader can now take it.** `sgp4.wgsl` has the branch and
  is measured above, but `UpdateRoster()` still filters those objects out and `UpdateDeepSpace()`
  still steps them on the CPU — correctly, in double, but one object at a time. Dropping the filter
  and deleting that function is all that is left, and nothing blocks it now that the refusal is
  gone. They are not a rounding error in this catalogue: `celestrak.py` curates `geo`, `gnss` and
  `gps-ops`, and the served query is unfiltered.
- **`SGP4Error::ResonanceStepLimitExceeded` has a case but no sweep coverage.** By construction —
  no verification element set gets within 58 steps of the limit, so the only thing exercising it is
  the dedicated case, at a time chosen to sit either side of the boundary.
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
