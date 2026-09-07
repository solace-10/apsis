#pragma once

#include <chrono>
#include <cmath>

#include <glm/vec3.hpp>

namespace WingsOfSteel
{

class OrbitalElementsComponent;

// The half of SGP4 that does not depend on time.
//
// SGP4 divides cleanly in two. Initialisation turns one element set into a block of coefficients,
// once, and the step then turns those coefficients plus a time since epoch into a position. On the
// near-earth path the step reads nothing else and writes nothing back, so the coefficients can be
// computed once on the CPU and handed to something else entirely to be stepped.
//
// That is the whole reason this half is here. WGSL has no f64 and SGP4 is a double precision
// algorithm, so the less of it that runs in f32 the smaller the error the shader has to answer
// for. Initialisation is also the part that pays off least on the GPU - it runs once per object
// rather than once per object per frame.
//
// Deep space fits the same split, which is not obvious and is why it went unimplemented for so
// long. Element sets with a period of 225 minutes or more are SGP4Method::DeepSpace and carry an
// SDP4Terms block as well as the one below; SDP4 adds lunar-solar periodics and Earth resonance
// terms, and the two parts of it that look like they need state carried between calls turn out
// not to. SDP4Terms says which and why. Just under an eighth of the catalogue - see
// game/tests/NOTES.md.
//
// Written from the published algorithm (Spacetrack Report #3, and Vallado, Crawford, Hujsak and
// Kelso, Revisiting Spacetrack Report #3, AIAA 2006-6753) rather than adapted from Vallado's own
// source, which is vendored under game/tests/reference/sgp4/ and stays there: it is the yardstick
// the tests measure this against, and its licence position is unresolved, so it never reaches a
// shipped binary. See CREDITS.md.
//
// Checked coefficient by coefficient against that reference over all 33 of the published
// verification element sets, in game/tests/space/sgp4_init_tests.cpp.

// The WGS72 gravity model, which is the one TLEs are fitted with: an element set propagated with
// any other set of constants is being propagated with constants its own fit did not use. Stated
// here rather than in the .cpp because the step needs four of them as well, and the step is going
// to end up on the GPU.
//
// Deliberately not earth_frame.hpp's kEarthSemiMajorAxis: that is WGS84's 6378.137 and is the
// right radius for placing a point on the ground, where this is the radius the propagator's own
// arithmetic is defined in terms of. The two metres between them belong to different questions.
inline constexpr double kSGP4EarthRadius = 6378.135; // km
inline constexpr double kSGP4Mu = 398600.8; // km^3/s^2
inline constexpr double kSGP4J2 = 0.001082616;
inline constexpr double kSGP4J3 = -0.00000253881;
inline constexpr double kSGP4J4 = -0.00000165597;
inline constexpr double kSGP4J3OverJ2 = kSGP4J3 / kSGP4J2;

// The square root of Earth's gravitational parameter in earth radii per minute, which is the unit
// system the whole algorithm works in. Not constexpr because std::sqrt is not, before C++26.
inline const double kSGP4Xke = 60.0 / std::sqrt(kSGP4EarthRadius * kSGP4EarthRadius * kSGP4EarthRadius / kSGP4Mu);

enum class SGP4Method
{
    NearEarth,
    DeepSpace
};

// One element set, in the units the algorithm itself is written in rather than the ones a GP
// record states. MakeSGP4Elements() converts; nothing else should have to.
struct SGP4Elements
{
    double bstar{ 0.0 }; // 1 / earth radii
    double ecco{ 0.0 }; // dimensionless
    double argpo{ 0.0 }; // radians
    double inclo{ 0.0 }; // radians
    double mo{ 0.0 }; // radians
    double nodeo{ 0.0 }; // radians
    double noKozai{ 0.0 }; // radians / minute, still Kozai's mean motion

    // Days since 1950 January 0.0 UTC. Read only by the deep space initialisation, which needs the
    // sidereal time at epoch; carried here so that the input to the two branches is one type.
    double epochDaysSince1950{ 0.0 };
};

// Which Earth resonance a deep-space orbit is in, if any.
//
// Vallado's irez, with his values kept because sgp4.wgsl mirrors them the way it mirrors
// SGP4Error's. The two arms share nothing: they are different sets of coefficients, computed by
// different arithmetic, and the step integrates one or the other.
enum class SDP4Resonance
{
    None = 0,

    // One turn a day, so the same longitudes pass beneath the object on every orbit and the
    // Earth's tesseral harmonics accumulate instead of averaging away. Geostationary.
    Synchronous = 1,

    // Two turns a day, on an orbit eccentric enough for it to matter. Molniya and Tundra.
    HalfDay = 2,
};

// The rest of the coefficient block, for the element sets SDP4 owns.
//
// Zero throughout unless method is DeepSpace. Nested rather than flattened into SGP4ElementSet so
// that the near-earth block stays the length it was and can still be read against the papers
// alongside the near-earth half of the step.
//
// The names are Vallado's, for the reason given below SGP4ElementSet.
//
// TWO THINGS THE REFERENCE STORES AND THIS DOES NOT, both of which are why deep space was thought
// not to fit the CPU-initialise / GPU-step split at all.
//
// atime, xli and xni - the resonance integrator's position. The reference carries them in its
// element record between calls, which would make the step stateful and so unable to run on the
// GPU. They are a cache rather than a semantic: the integration restarts from atime = 0 whenever
// the time changes sign or moves nearer the epoch, and otherwise walks fixed 720 minute steps
// towards the time asked for. The states reachable from zero are therefore a fixed sequence, and
// stopping at any one of them is the same arithmetic in the same order however you got there.
// Restarting on every call is bit-identical and costs |t| / 720 iterations, which is three at the
// catalogue's median object age and five at its ninetieth percentile - and nothing at all for a
// non-resonant orbit, which is most of them. sgp4_step_tests.cpp measures this rather than
// assuming it: the reference walks each verification case with its cache warm while ours restarts
// at every time, and the two still agree exactly.
//
// peo, pgho, pho, pinco and plo. dscom sets all five to zero and dpper only ever subtracts them,
// so they are identically zero in the reference as well - five dead doubles per object in a block
// that gets uploaded to the GPU for the whole catalogue. sgp4_init_tests.cpp asserts the
// reference's own five are zero, so this is checked rather than believed.
struct SDP4Terms
{
    SDP4Resonance resonance{ SDP4Resonance::None };

    // The rates at which the Sun and Moon move the mean elements, radians per minute. These are
    // secular: the step multiplies them by the elapsed time.
    double dedt{ 0.0 };
    double didt{ 0.0 };
    double dmdt{ 0.0 };
    double dnodt{ 0.0 };
    double domdt{ 0.0 };

    // The lunar-solar periodics. Amplitudes of terms in the Sun's and Moon's own positions, which
    // the step evaluates afresh at every call rather than accumulating - which is what makes them
    // periodic rather than secular.
    double e3{ 0.0 };
    double ee2{ 0.0 };
    double se2{ 0.0 };
    double se3{ 0.0 };
    double sgh2{ 0.0 };
    double sgh3{ 0.0 };
    double sgh4{ 0.0 };
    double sh2{ 0.0 };
    double sh3{ 0.0 };
    double si2{ 0.0 };
    double si3{ 0.0 };
    double sl2{ 0.0 };
    double sl3{ 0.0 };
    double sl4{ 0.0 };
    double xgh2{ 0.0 };
    double xgh3{ 0.0 };
    double xgh4{ 0.0 };
    double xh2{ 0.0 };
    double xh3{ 0.0 };
    double xi2{ 0.0 };
    double xi3{ 0.0 };
    double xl2{ 0.0 };
    double xl3{ 0.0 };
    double xl4{ 0.0 };

    // Where the Moon and the Sun were at the epoch. The periodics above are evaluated against
    // these advanced to the time asked for.
    double zmol{ 0.0 };
    double zmos{ 0.0 };

    // The Earth resonance terms. HalfDay uses the ten d-coefficients and Synchronous the three
    // del-coefficients, so at most one group is ever non-zero; not worth a union, since the whole
    // block is uploaded as one either way.
    double d2201{ 0.0 };
    double d2211{ 0.0 };
    double d3210{ 0.0 };
    double d3222{ 0.0 };
    double d4410{ 0.0 };
    double d4422{ 0.0 };
    double d5220{ 0.0 };
    double d5232{ 0.0 };
    double d5421{ 0.0 };
    double d5433{ 0.0 };
    double del1{ 0.0 };
    double del2{ 0.0 };
    double del3{ 0.0 };

    // Greenwich sidereal time at the epoch, which is what ties a resonance to a place on the
    // ground rather than to a direction in space, and the two values the integration starts from.
    double gsto{ 0.0 };
    double xfact{ 0.0 };
    double xlamo{ 0.0 };
};

// Everything the step reads, and nothing else.
//
// The coefficient names are Vallado's, unchanged and deliberately so. They are the names used by
// Spacetrack Report #3, by the AIAA paper, by the reference implementation and by every other
// port of it; renaming them to something self-describing would make this code impossible to read
// alongside the source it was derived from, and checking it is the only way anyone can be sure it
// is right.
struct SGP4ElementSet
{
    SGP4Method method{ SGP4Method::NearEarth };

    // Vallado's isimp. The drag model drops its higher order terms for orbits that decay fast
    // enough not to benefit from them - a perigee below 220 km - and for deep space.
    bool simplifiedDrag{ false };

    // Carried through from the input: the step reads these at every call, not just at init.
    double bstar{ 0.0 };
    double ecco{ 0.0 };
    double inclo{ 0.0 };
    double nodeo{ 0.0 };
    double argpo{ 0.0 };
    double mo{ 0.0 };

    // The mean motion with Kozai's secular J2 correction removed, radians / minute.
    double no_unkozai{ 0.0 };

    double aycof{ 0.0 };
    double con41{ 0.0 };
    double cc1{ 0.0 };
    double cc4{ 0.0 };
    double cc5{ 0.0 };
    double d2{ 0.0 };
    double d3{ 0.0 };
    double d4{ 0.0 };
    double delmo{ 0.0 };
    double eta{ 0.0 };
    double argpdot{ 0.0 };
    double omgcof{ 0.0 };
    double sinmao{ 0.0 };
    double t2cof{ 0.0 };
    double t3cof{ 0.0 };
    double t4cof{ 0.0 };
    double t5cof{ 0.0 };
    double x1mth2{ 0.0 };
    double x7thm1{ 0.0 };
    double mdot{ 0.0 };
    double nodedot{ 0.0 };
    double xlcof{ 0.0 };
    double xmcof{ 0.0 };
    double nodecf{ 0.0 };

    // Zero unless method is DeepSpace. The step reads it only on that branch.
    SDP4Terms deepSpace;
};

// Turns an element set into the coefficients the step needs.
//
// Total, in the sense that it has no failure mode: the reference removed initialisation's input
// checks, and every error SGP4 reports - eccentricity out of range, a decayed orbit - is raised by
// the step instead, where it can be raised against a particular time. An element set this cannot
// make sense of comes back with its coefficients zeroed rather than flagged.
SGP4ElementSet SGP4Initialise(const SGP4Elements& elements);

// Why a step failed. SGP4 raises these against a particular time rather than against the element
// set, which is why initialisation has no failure mode of its own and this does.
//
// The numbering is Vallado's, minus his 5, which is commented out at initialisation in the
// reference itself. Ours continue from where his stop.
//
// The values are stated rather than left implicit because sgp4.wgsl mirrors them: the shader has
// no way to return an enum, so it writes one of these numbers alongside the position.
enum class SGP4Error
{
    None = 0,

    // Deep space reached something that cannot step it. SGP4Step() no longer raises this, because
    // it can now step deep space; sgp4.wgsl still does, because its deep-space branch has not been
    // written yet and refusing is better than propagating an element set it only half understands.
    // So this currently means "not supported here" rather than "not supported", and it goes when
    // the shader catches up.
    //
    // The value would stay even then. sgp4.wgsl mirrors these numbers and PropagationFailureComponent
    // latches them, so renumbering would silently relabel every error either of them has recorded.
    DeepSpaceNotSupported = 1,

    MeanMotionNotPositive = 2, // Vallado 2
    MeanElementsOutOfRange = 3, // Vallado 1: eccentricity has left [-0.001, 1), or am below 0.95
    NegativeSemiLatusRectum = 4, // Vallado 4
    Decayed = 5, // Vallado 6: the orbit has come down inside the Earth

    // Ours, with no counterpart in the reference: the drag correction to the semi-major axis has
    // changed sign.
    //
    //   tempa = 1 - cc1*t - d2*t^2 - d3*t^3 - d4*t^4
    //   am    = (xke/no_unkozai)^(2/3) * tempa^2
    //
    // tempa is a truncated polynomial. Near epoch it is about 1, but far enough out on a high-drag
    // orbit the t^4 term takes over and drives it negative - and because am squares it, the sign
    // disappears. A wildly negative tempa becomes a wildly LARGE semi-major axis rather than a
    // small one, so the object is flung outwards; nm is then recomputed from that same huge am and
    // comes out small, which is why the symptom is an object crossing the screen at an impossible
    // speed while reporting a plausible velocity.
    //
    // The checks already here catch it only by accident, and only briefly. Just past the sign
    // change am is still small, so the semi-latus rectum or mrt can trip on it - 29141 crossing
    // zero at t = 1392 minutes does raise the reference's error 4. But am is a0*tempa^2 and climbs
    // back up through healthy values as the correction keeps growing, and the checks lose it
    // again: by t = 2784 the same element set returns success from the reference, at a radius of
    // 1.09 million km and a velocity of 0.6 km/s. Over the live catalogue at 2026-09-06 exactly
    // two of 28,531 near-earth objects were past the sign change, at tempa -9.24 and -2.99 - well
    // outside that window, and exactly the two that were visibly wrong on screen.
    //
    // Why the reference does not test for this is worth setting down, because adding a check the
    // reference does not make is the kind of thing that is usually a misreading on our part rather
    // than a gap in a model this well used. Three things are visible in his code, and none of them
    // need him to have got anything wrong.
    //
    // A check on the neighbouring failure exists and is disabled. His documented error 1 reads
    // "ecc >= 1.0 or ecc < -0.001 or a < 0.95 er", but the a < 0.95 half is commented out, noted
    // "sgp4fix am is fixed from the previous nm check". Taken literally the nm checked there is
    // no_unkozai, a constant from initialisation, and we cannot see how it constrains tempa - but
    // that may well be shorthand for something established elsewhere in work we have not read, and
    // it is a potential oversight at most rather than a fault we are in a position to call. It
    // would not catch this case in any event: it is a floor, and this failure sends am upwards.
    //
    // The sgp4fix series is also deliberately permissive, letting element sets "process until they
    // are actually below earth surface" rather than refusing early - which leaves judging the
    // output to the caller, and this check is us doing that.
    //
    // And the regime sits outside the envelope the model is verified over: across every near-earth
    // case in SGP4-VER.TLE, over each one's full published range, tempa never falls below 0.951.
    // Reaching here takes a stale element set on a decaying object, and the operational answer to
    // that is a fresher element set rather than an error code. We do not have that option, since
    // the catalogue is whatever the server last ingested, so the check falls to us.
    //
    // Zero is the threshold because it is the only one that needs no justification - the correction
    // has changed sign - and because everything above it is already covered. am is a0*tempa^2 with
    // a0 about 1.065 in low earth orbit, so a collapsing tempa drops mrt below one earth radius at
    // about 0.97 and Decayed catches it from there down. Testing tempa rather than what tempa
    // produces is the point: am, mrt and the semi-latus rectum all recover as the divergence
    // worsens, and tempa does not.
    //
    // Mirrored in sgp4.wgsl as kErrorDragModelDiverged.
    DragModelDiverged = 6,

    // Vallado 3, raised inside dpper and so only reachable on the deep-space branch: the
    // lunar-solar periodics have moved the eccentricity outside [0, 1], which is not an orbit.
    //
    // Numbered 7 rather than 3 because 3 is already taken. His error codes are an enumeration of
    // failures, not an ordering of them, and ours has to stay compatible with what sgp4.wgsl and
    // PropagationFailureComponent have already written down; sgp4_step_tests.cpp maps between the
    // two, which is the only place the difference matters.
    PerturbedEccentricityOutOfRange = 7,
};

// Where an object is, and how fast, at one instant.
//
// TEME - the frame the mean elements are expressed in and the one CalculateGMST() pairs with.
// See the note above ECIToECEF() in earth_frame.hpp about why positions are labelled ECI anyway.
struct SGP4Position
{
    glm::dvec3 position{ 0.0 }; // km
    glm::dvec3 velocity{ 0.0 }; // km/s
    SGP4Error error{ SGP4Error::None };
};

// Propagates an initialised element set to a time, in minutes from its epoch.
//
// The other half of SGP4, and the half that will end up in the shader: it reads nothing but the
// coefficients and the time, writes nothing back, and carries nothing between calls. That holds on
// the deep-space branch too, which is not free - see SDP4Terms - and is what keeps both branches
// able to run anywhere. Negative times propagate backwards and are as valid as positive ones.
//
// This exists in double, on the CPU, so that the shader has something of ours to be compared
// against. A WGSL step measured against the reference would be measuring transcription mistakes
// and f32 precision loss at once, with no way to tell which; measured against this, everything
// left is precision. See game/tests/NOTES.md.
//
// A failed step still returns whatever it had computed. That distinction is the reference's:
// eccentricity, mean motion and semi-latus rectum failures give up before there is a position to
// give, but a decayed orbit is detected from the position, which is therefore present.
SGP4Position SGP4Step(const SGP4ElementSet& elementSet, double tsinceMinutes);

// Converts a served GP record into the algorithm's units: degrees to radians, revolutions per day
// to radians per minute, and an epoch to days since 1950.
//
// The component stores floats, so about seven significant digits reach an algorithm written in
// double. Mean motion dominates that loss and it is worth roughly twenty metres of along-track
// error after a day - two orders of magnitude inside SGP4's own accuracy, and far inside what the
// f32 step will contribute.
SGP4Elements MakeSGP4Elements(const OrbitalElementsComponent& orbitalElements);

} // namespace WingsOfSteel
