#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "reference/sgp4_reference.hpp"
#include "space/sgp4.hpp"

using namespace WingsOfSteel;

namespace
{

const std::string kTleFile = std::string(APSIS_TEST_DATA_DIR) + "/SGP4-VER.TLE";

// What the sweep below should cover, stated rather than counted so that a loop which quietly stops
// covering something fails instead of passing over less.
//
// 669 positions against 667 rows in tcppver.out, and the difference is two things that cancel
// unevenly. Three cases end on a decayed orbit, which still has a position - that is how it was
// noticed - so each contributes one comparison past its last published row: 28872, 29141 and,
// 3.5 years into its long run, 20413. Against that, 33334 fails at its own epoch, so the single
// row the file prints for it is never reached; NOTES.md records that the row is stale output from
// the satellite before it and is not a position for 33334 at all.
constexpr int kExpectedComparisons = 669;

// 22312 and 28350 on the eccentricity, 28872, 29141 and 20413 by decaying, 33333 on the
// semi-latus rectum, and 33334 at its own epoch on the perturbed eccentricity - which is the one
// failure only the deep-space branch can raise, and the reason SGP4Error needed a new value.
constexpr int kExpectedFailedCases = 7;

// The reference reports its failures as an int on the satrec. All of them are reachable now that
// deep space is; only 5 is not, being commented out at initialisation in the reference itself.
//
// The numbering diverges at 3, which is ours only because his 3 was taken by the time we needed
// one. This is the only place that matters, and it is why SGP4Error states its values.
SGP4Error AsError(int referenceError)
{
    switch (referenceError)
    {
    case 0:
        return SGP4Error::None;
    case 1:
        return SGP4Error::MeanElementsOutOfRange;
    case 2:
        return SGP4Error::MeanMotionNotPositive;
    case 3:
        return SGP4Error::PerturbedEccentricityOutOfRange;
    case 4:
        return SGP4Error::NegativeSemiLatusRectum;
    case 6:
        return SGP4Error::Decayed;
    default:
        FAIL("The reference raised error " << referenceError << ", which nothing should be able to reach");
        return SGP4Error::None;
    }
}

Test::VerificationCase FindCase(const std::string& satnum)
{
    for (const Test::VerificationCase& verificationCase : Test::LoadVerificationCases(kTleFile))
    {
        if (verificationCase.satrec.satnum == satnum)
        {
            return verificationCase;
        }
    }

    FAIL("SGP4-VER.TLE has no element set for satellite " << satnum);
    return Test::VerificationCase{};
}

} // namespace

// The case the shader will be measured against.
//
// SGP4Step() exists so that the WGSL version has something of ours to be compared with. A shader
// checked straight against the reference would be measuring transcription mistakes and f32
// precision loss added together, with no way to say which had gone wrong; checked against this,
// once this agrees with the reference exactly, everything left over is precision.
//
// So this is the case that has to be exact rather than close. Every element set in the
// verification file, at every time it asks to be propagated to, position and velocity both.
//
// It is also, without needing a case of its own, the measurement behind the claim at SDP4Terms
// that the deep-space resonance integrator needs no state carried between calls. The reference is
// used the way it is meant to be - one element record stepped forward through its range, its
// integrator resuming from wherever the previous call left it - while ours is handed a const
// coefficient block and restarts the integration from the epoch at every single time. If those two
// were not the same arithmetic, the resonant cases would disagree, and they are twelve of the
// twenty-four here.
TEST_CASE("Our step reproduces the reference propagator", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    REQUIRE(cases.size() > 25);

    int nearEarthCases = 0;
    int deepSpaceCases = 0;
    int failedCases = 0;
    int comparisons = 0;

    for (const Test::VerificationCase& verificationCase : cases)
    {
        // The reference's own answer for which algorithm this element set belongs to, rather than
        // a threshold restated here.
        (verificationCase.satrec.method == 'd' ? deepSpaceCases : nearEarthCases)++;

        INFO("satellite " << verificationCase.satnum);

        // One satrec stepped repeatedly, as the reference itself is used: this is the sequence
        // that produced tcppver.out, and on the deep-space cases it is also the sequence that
        // keeps its integrator's cache warm.
        elsetrec satrec = verificationCase.satrec;
        const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));

        for (const double tsince : Test::VerificationTimes(verificationCase))
        {
            INFO("t = " << tsince << " minutes");

            double r[3];
            double v[3];
            SGP4Funcs::sgp4(satrec, tsince, r, v);

            const SGP4Position stepped = SGP4Step(elementSet, tsince);
            CHECK(stepped.error == AsError(satrec.error));

            // A decayed orbit has a position - that is how it was noticed - where the other
            // failures give up before there is one. Comparing what the reference never wrote
            // would be comparing uninitialised memory.
            if (satrec.error == 0 || satrec.error == 6)
            {
                CHECK(stepped.position.x == r[0]);
                CHECK(stepped.position.y == r[1]);
                CHECK(stepped.position.z == r[2]);
                CHECK(stepped.velocity.x == v[0]);
                CHECK(stepped.velocity.y == v[1]);
                CHECK(stepped.velocity.z == v[2]);
                ++comparisons;
            }

            if (satrec.error != 0)
            {
                ++failedCases;
                break;
            }
        }
    }

    // So that the loop cannot quietly stop covering anything.
    CHECK(nearEarthCases == 9);
    CHECK(deepSpaceCases == 24);
    CHECK(comparisons == kExpectedComparisons);

    // Some cases give up before the end of the range they ask for. That is not incidental
    // coverage: it means the agreement above is agreement about where the propagation stops being
    // valid as well as about where the object is, and those are separate ways to be wrong.
    CHECK(failedCases == kExpectedFailedCases);
}

// Four near-earth cases fail partway, and the case above already checks that ours fails where the
// reference does. This one pins the decay to a time rather than to an agreement: 29141's published
// range brackets its re-entry tightly enough to say which step it happens on.
TEST_CASE("A decaying orbit stops where the reference stops", "[space][sgp4]")
{
    // 29141 is asked for 0 to 440 minutes in steps of 20, which is 23 rows counting the epoch.
    // tcppver.out holds 22: it comes down between 420 and 440.
    const Test::VerificationCase verificationCase = FindCase("29141");
    REQUIRE(verificationCase.satrec.method == 'n');

    const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));

    double lastGoodTime = -1.0;
    double decayTime = -1.0;

    for (const double tsince : Test::VerificationTimes(verificationCase))
    {
        const SGP4Position stepped = SGP4Step(elementSet, tsince);
        if (stepped.error == SGP4Error::None)
        {
            lastGoodTime = tsince;
            continue;
        }

        CHECK(stepped.error == SGP4Error::Decayed);
        decayTime = tsince;
        break;
    }

    CHECK(lastGoodTime == 420.0);
    CHECK(decayTime == 440.0);
}

// The one check ours makes that the reference does not, and the case that shows why.
//
// 29141 is the verification file's own fast-decaying object, and no element set in that file
// reaches this within the range it asks to be propagated over - the minimum tempa across all nine
// near-earth cases is 0.951. Taken past its published range, though, its drag polynomial changes
// sign, and what the reference does with that is the whole argument for the check: at 2784 minutes
// from epoch, under two days, it returns success at nearly three times the distance to the Moon
// while reporting a velocity no orbit out there could have. That is exactly the symptom this was
// found by - an object crossing the screen at an impossible speed while claiming a plausible one.
//
// See SGP4Error::DragModelDiverged for why the reference does not object to it.
TEST_CASE("A diverged drag model is rejected where the reference reports success", "[space][sgp4]")
{
    const Test::VerificationCase verificationCase = FindCase("29141");
    REQUIRE(verificationCase.satrec.method == 'n');

    elsetrec satrec = verificationCase.satrec;
    const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));

    // Where tempa has reached about -12.8. Well past the sign change rather than at it, which is
    // the regime the two objects that prompted the check were in.
    constexpr double kDivergedTime = 2784.0;

    double r[3];
    double v[3];
    REQUIRE(SGP4Funcs::sgp4(satrec, kDivergedTime, r, v));
    REQUIRE(satrec.error == 0);
    CHECK(std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]) > 1.0e6);
    CHECK(std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) < 1.0);

    CHECK(SGP4Step(elementSet, kDivergedTime).error == SGP4Error::DragModelDiverged);

    // And at the crossing itself, where the reference does object - to the semi-latus rectum,
    // having reached it by a different route. Pinned because it is the reason the check is on
    // tempa rather than on what tempa produces: this is the narrow window in which the existing
    // checks happen to work, and kDivergedTime above is what lies past it.
    satrec = verificationCase.satrec;
    SGP4Funcs::sgp4(satrec, 1392.0, r, v);
    CHECK(satrec.error == 4);
    CHECK(SGP4Step(elementSet, 1392.0).error == SGP4Error::DragModelDiverged);
}

// A deep-space element set steps, and the order it is stepped in does not matter.
//
// The case above already compares every deep-space position against the reference, but it walks
// each range forwards, which is the one order in which a resonance integrator that did carry state
// between calls would also be right. This one asks for the same times out of order, backwards, and
// twice over, on the orbit whose resonance is strongest.
//
// If SGP4Step() were not pure this is what would catch it - and being pure is the whole reason it
// can be handed to a compute shader that runs thirty thousand of these in an arbitrary order.
// The second check ours makes that the reference does not, and - like the drag one above - a case
// the verification sweep cannot reach. Across every element set in the file at every published
// time, the resonance integrator's worst is 14 steps of the 72 it is allowed.
//
// The limit is really a limit on how far from its epoch a resonant element set may be propagated.
// The integrator walks 720-minute steps from the epoch, and it stops when the remaining time is
// under one step, so it reaches any |t| below (72 + 1) * 720 = 52,560 minutes and no further. That
// is about 36 days, against a catalogue that cannot hold an element set older than about 33 - see
// kResonanceMaxSteps in sgp4.cpp for where those numbers come from.
//
// See SGP4Error::ResonanceStepLimitExceeded for why a limit exists at all: sgp4.wgsl cannot have an
// unbounded loop, and a bound the shader kept and this did not would stop the two being comparable.
TEST_CASE("A resonance too far from its epoch is refused where the reference reports success", "[space][sgp4]")
{
    // 25954 again: a real geostationary satellite, so the 1:1 resonance and the integrator running.
    const Test::VerificationCase verificationCase = FindCase("25954");
    const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));
    REQUIRE(elementSet.deepSpace.resonance == SDP4Resonance::Synchronous);

    constexpr double kLastReachable = 52559.0;
    constexpr double kFirstRefused = 52560.0;

    // Either side of the boundary, and the reference propagating cleanly across both - it has no
    // limit of its own, so this is our refusal rather than a failure either of us has detected.
    double r[3];
    double v[3];
    for (const double tsince : { kLastReachable, kFirstRefused })
    {
        INFO("t = " << tsince << " minutes");
        elsetrec satrec = verificationCase.satrec;
        REQUIRE(SGP4Funcs::sgp4(satrec, tsince, r, v));
        REQUIRE(satrec.error == 0);
    }

    CHECK(SGP4Step(elementSet, kLastReachable).error == SGP4Error::None);
    CHECK(SGP4Step(elementSet, kFirstRefused).error == SGP4Error::ResonanceStepLimitExceeded);

    // Backwards as well, since the step takes its sign from the time and the count is on |t|.
    CHECK(SGP4Step(elementSet, -kLastReachable).error == SGP4Error::None);
    CHECK(SGP4Step(elementSet, -kFirstRefused).error == SGP4Error::ResonanceStepLimitExceeded);

    // And a non-resonant deep-space element set is untouched by any of it: nothing integrates, so
    // there is no step count to exceed however far out it is asked for.
    const SGP4ElementSet nonResonant = SGP4Initialise(Test::AsElements(FindCase("20413").satrec));
    REQUIRE(nonResonant.method == SGP4Method::DeepSpace);
    REQUIRE(nonResonant.deepSpace.resonance == SDP4Resonance::None);
    CHECK(SGP4Step(nonResonant, kFirstRefused).error == SGP4Error::None);
}

TEST_CASE("A deep-space step depends on nothing but its arguments", "[space][sgp4]")
{
    // 25954, a real geostationary satellite, so a 1:1 resonance and the integrator running.
    const Test::VerificationCase verificationCase = FindCase("25954");
    REQUIRE(verificationCase.satrec.method == 'd');

    const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));
    REQUIRE(elementSet.method == SGP4Method::DeepSpace);
    REQUIRE(elementSet.deepSpace.resonance == SDP4Resonance::Synchronous);

    // Well past the integrator's 720 minute step, so several passes of it are involved, and either
    // side of the epoch, so the sign of the step is too.
    const std::vector<double> times = { 0.0, 1440.0, -2880.0, 4320.0, 60.0, -1440.0, 2880.0 };

    std::vector<SGP4Position> forwards;
    for (const double tsince : times)
    {
        forwards.push_back(SGP4Step(elementSet, tsince));
    }

    for (size_t i = times.size(); i-- > 0;)
    {
        INFO("t = " << times[i] << " minutes");
        const SGP4Position again = SGP4Step(elementSet, times[i]);
        CHECK(again.error == forwards[i].error);
        CHECK(again.position == forwards[i].position);
        CHECK(again.velocity == forwards[i].velocity);
    }

    // And it went somewhere real: geostationary, so a little over 42,000 km from the centre.
    CHECK(forwards.front().error == SGP4Error::None);
    CHECK_THAT(glm::length(forwards.front().position), Catch::Matchers::WithinRel(42164.0, 1.0e-3));
}
