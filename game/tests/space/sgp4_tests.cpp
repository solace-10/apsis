#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "components/orbital_elements_component.hpp"
#include "reference/sgp4_reference.hpp"
#include "systems/orbit_propagation_system.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;

namespace
{

const std::string kTleFile = std::string(APSIS_TEST_DATA_DIR) + "/SGP4-VER.TLE";
const std::string kOutputFile = std::string(APSIS_TEST_DATA_DIR) + "/tcppver.out";

struct PropagationRun
{
    std::vector<Test::VerificationStep> steps;
    int error{ 0 };
};

// Reproduces the exact sequence of calls that produced tcppver.out: one at the epoch, then
// the range the element set asks for, stopping the moment sgp4 reports an error.
//
// The awkward parts are load bearing rather than stylistic. The epoch call is separate
// because the published first row is always at t=0 regardless of where the range starts -
// which is why the negative ranges (04632, 09998) still open with one. The step back before
// the loop is what stops a range starting at zero printing its first row twice. And the
// clamp to the stop time is why a range that does not divide evenly by its step still ends
// exactly on it.
PropagationRun Propagate(const Test::VerificationCase& verificationCase)
{
    elsetrec satrec = verificationCase.satrec;
    PropagationRun run;

    double r[3];
    double v[3];

    SGP4Funcs::sgp4(satrec, 0.0, r, v);
    if (satrec.error != 0)
    {
        // The published row at this point is stale output left over from the previous
        // element set, not a result for this one, so there is nothing here to compare.
        run.error = satrec.error;
        return run;
    }
    run.steps.push_back(Test::VerificationStep{ 0.0, { r[0], r[1], r[2] }, { v[0], v[1], v[2] } });

    double tsince = verificationCase.startMinutes;
    if (std::abs(tsince) > 1.0e-8)
    {
        tsince -= verificationCase.stepMinutes;
    }

    while (tsince < verificationCase.stopMinutes)
    {
        tsince += verificationCase.stepMinutes;
        if (tsince > verificationCase.stopMinutes)
        {
            tsince = verificationCase.stopMinutes;
        }

        SGP4Funcs::sgp4(satrec, tsince, r, v);
        if (satrec.error != 0)
        {
            run.error = satrec.error;
            break;
        }

        run.steps.push_back(Test::VerificationStep{ tsince, { r[0], r[1], r[2] }, { v[0], v[1], v[2] } });
    }

    return run;
}

// A TLE zero-pads the satellite number into its five columns; the output file prints it as
// a number. Comparing them without this would be comparing formatting, not identity.
std::string StripLeadingZeroes(const std::string& satnum)
{
    const size_t first = satnum.find_first_not_of('0');
    return first == std::string::npos ? "0" : satnum.substr(first);
}

// The verification file is small enough that re-reading it per case costs nothing and
// keeps each one able to run on its own.
Test::VerificationCase FindCase(const std::string& satnum)
{
    for (const Test::VerificationCase& verificationCase : Test::LoadVerificationCases(kTleFile))
    {
        if (StripLeadingZeroes(verificationCase.satnum) == StripLeadingZeroes(satnum))
        {
            return verificationCase;
        }
    }

    FAIL("SGP4-VER.TLE has no element set for satellite " << satnum);
    return Test::VerificationCase{};
}

PropagationRun RunCase(const std::string& satnum)
{
    return Propagate(FindCase(satnum));
}

} // namespace

// The point of this case is not that SGP4 is correct - that is Vallado's problem, and the
// published output is his answer. It is that the copy of it sitting in this tree is the one
// he published, built the way we build it. Everything else in the suite that compares
// against this propagator is worth exactly as much as this case passing.
TEST_CASE("The vendored SGP4 reference reproduces Vallado's published verification output", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    const std::vector<Test::VerificationBlock> blocks = Test::LoadVerificationOutput(kOutputFile);

    REQUIRE(cases.size() == blocks.size());
    REQUIRE(cases.size() > 25); // A truncated fetch of either file would otherwise pass quietly.

    for (size_t caseIndex = 0; caseIndex < cases.size(); caseIndex++)
    {
        const Test::VerificationCase& verificationCase = cases[caseIndex];
        const Test::VerificationBlock& block = blocks[caseIndex];

        INFO("satellite " << verificationCase.satnum);
        REQUIRE(StripLeadingZeroes(verificationCase.satnum) == StripLeadingZeroes(block.satnum));

        const PropagationRun run = Propagate(verificationCase);

        // An element set that fails at its own epoch has no results of its own in the
        // file - the single row printed under it is stale output carried over from the
        // previous satellite - so it is covered by its own case below rather than here.
        if (run.steps.empty())
        {
            continue;
        }

        // Not just a bound on the loop: the row count is where the propagation gave up, so
        // requiring it to match pins the point at which an element set starts failing.
        REQUIRE(run.steps.size() == block.steps.size());

        for (size_t stepIndex = 0; stepIndex < run.steps.size(); stepIndex++)
        {
            const Test::VerificationStep& step = run.steps[stepIndex];
            const Test::VerificationStep& expected = block.steps[stepIndex];

            INFO("satellite " << verificationCase.satnum << " at t=" << step.tsince);
            REQUIRE_THAT(step.tsince, WithinAbs(expected.tsince, 1e-8));

            // The file prints eight decimals of a kilometre and nine of a kilometre per
            // second, so it cannot resolve below 5e-9 km and 5e-10 km/s. Every case here
            // agrees to within that - which is to say, to the last digit the file has -
            // except the 3.5-year propagation of 20413, which reaches 1.2e-7 km. The
            // tolerances are set an order of magnitude above the worst observed, so they
            // measure our build against Vallado's rather than the printing.
            for (int axis = 0; axis < 3; axis++)
            {
                REQUIRE_THAT(step.position[axis], WithinAbs(expected.position[axis], 1e-6));
                REQUIRE_THAT(step.velocity[axis], WithinAbs(expected.velocity[axis], 1e-9));
            }
        }
    }
}

// SGP4 reports a satellite it cannot propagate rather than returning a plausible-looking
// position, and the last few element sets in SGP4-VER.TLE exist to trip exactly that. The
// codes are pinned here because the compute path will have to make the same distinction,
// and silently propagating a decayed object is the failure that looks like success.
//
// Split across cases rather than sections for the same reason the suite registers one CTest
// entry per case: a trip that takes the process down should not take the others with it.
TEST_CASE("A satellite whose orbit decays partway through the range stops there", "[space][sgp4]")
{
    const PropagationRun run = RunCase("33333");
    REQUIRE(run.error == 4); // Semi-latus rectum below zero.
    REQUIRE(run.steps.size() == 5);
}

TEST_CASE("An element set that fails at its own epoch produces no positions at all", "[space][sgp4]")
{
    const PropagationRun run = RunCase("33334");
    REQUIRE(run.error == 3); // Perturbed eccentricity below zero.
    REQUIRE(run.steps.empty());
}

// The file's own comment on this one reads "try to check error code 3 looks like ep never
// goes below zero". It does not, so the case is really a near-circular geostationary orbit
// that propagates cleanly, and that is what is pinned - a future change making it fail
// would be a regression, not the error the label suggests.
TEST_CASE("The near-circular case labelled an error attempt in fact propagates", "[space][sgp4]")
{
    const PropagationRun run = RunCase("33335");
    REQUIRE(run.error == 0);
}

// What the compute shader is being written to replace, measured rather than asserted about.
//
// CalculateCartesianPosition() takes SGP4 mean elements and propagates them as if they were
// classical Keplerian ones, so everything SGP4 exists to model - the J2 secular drifts above
// all - is simply absent. 06251 is a real low Earth orbit at 377 km perigee, which is where
// that costs the most.
//
// The bounds are a band rather than a ceiling on purpose. The lower one is what makes this a
// before-and-after marker: if the propagation is ever made correct without this case being
// revisited, it fails and says so, rather than passing more comfortably and saying nothing.
TEST_CASE("Two-body propagation of SGP4 mean elements drifts by a known amount", "[space][sgp4]")
{
    const Test::VerificationCase verificationCase = FindCase("06251");
    const OrbitalElementsComponent component = Test::AsComponent(verificationCase.satrec);

    elsetrec satrec = verificationCase.satrec;

    double errorAtEpoch = 0.0;
    double maxErrorOverADay = 0.0;

    for (int minutes = 0; minutes <= 1440; minutes += 10)
    {
        double r[3];
        double v[3];
        REQUIRE(SGP4Funcs::sgp4(satrec, static_cast<double>(minutes), r, v));

        const glm::dvec3 expected(r[0], r[1], r[2]);
        const glm::dvec3 actual = OrbitPropagationSystem::CalculateCartesianPosition(
            component, component.GetEpoch() + std::chrono::minutes(minutes));

        const double error = glm::length(actual - expected);
        if (minutes == 0)
        {
            errorAtEpoch = error;
        }
        maxErrorOverADay = std::max(maxErrorOverADay, error);
    }

    INFO("error at epoch " << errorAtEpoch << " km, worst over a day " << maxErrorOverADay << " km");

    // At the epoch itself only the short-period terms are missing, and they currently cost
    // about 13 km.
    REQUIRE(errorAtEpoch > 5.0);
    REQUIRE(errorAtEpoch < 30.0);

    // A day later the secular drift dominates: about 416 km, or a quarter of the way round
    // the Earth in ground track terms. That is the whole case for doing this properly.
    REQUIRE(maxErrorOverADay > 200.0);
    REQUIRE(maxErrorOverADay < 800.0);
}
