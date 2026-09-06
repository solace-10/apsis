#include <cmath>
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

// The reference reports its failures as an int on the satrec. Only these four are reachable on
// the near-earth path: 3 is raised inside dpper, which only deep space calls, and 5 is commented
// out at initialisation.
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
    case 4:
        return SGP4Error::NegativeSemiLatusRectum;
    case 6:
        return SGP4Error::Decayed;
    default:
        FAIL("The reference raised error " << referenceError << ", which the near-earth path should not be able to reach");
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
// So this is the case that has to be exact rather than close. Every near-earth element set in the
// verification file, at every time it asks to be propagated to, position and velocity both.
TEST_CASE("Our step reproduces the reference propagator over the near-Earth cases", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    REQUIRE(cases.size() > 25);

    int nearEarthCases = 0;
    int failedCases = 0;
    int comparisons = 0;

    for (const Test::VerificationCase& verificationCase : cases)
    {
        // The reference's own answer for which algorithm this element set belongs to, rather than
        // a threshold restated here.
        if (verificationCase.satrec.method != 'n')
        {
            continue;
        }

        ++nearEarthCases;
        INFO("satellite " << verificationCase.satnum);

        // One satrec stepped repeatedly, as the reference itself is used: on the near-earth path
        // nothing is carried between calls, and this is the sequence that produced tcppver.out.
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

    // So that the loop cannot quietly stop covering anything: nine cases and 160 positions
    // between them. Two more than tcppver.out publishes for these cases, because a decayed orbit
    // still has a position and there are two of those - the file stops at the last good row.
    CHECK(nearEarthCases == 9);
    CHECK(comparisons == 160);

    // Four of the nine give up before the end of the range they ask for - 22312, 28350, 28872 and
    // 29141. That is not incidental coverage: it means the agreement above is agreement about
    // where the propagation stops being valid as well as about where the object is, and those are
    // separate ways to be wrong.
    CHECK(failedCases == 4);
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

// The contract that makes a zeroed element set safe to hold. SGP4Component carries one of these
// for every deep-space object in the catalogue, and no_unkozai being zero means the first thing
// an unguarded step would do with it is divide by zero.
TEST_CASE("A deep-space element set cannot be stepped", "[space][sgp4]")
{
    // 25954, a real geostationary satellite.
    const Test::VerificationCase verificationCase = FindCase("25954");
    REQUIRE(verificationCase.satrec.method == 'd');

    const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(verificationCase.satrec));
    REQUIRE(elementSet.method == SGP4Method::DeepSpace);

    const SGP4Position stepped = SGP4Step(elementSet, 60.0);

    CHECK(stepped.error == SGP4Error::DeepSpaceNotSupported);
    CHECK(stepped.position == glm::dvec3(0.0));
    CHECK(stepped.velocity == glm::dvec3(0.0));
}
