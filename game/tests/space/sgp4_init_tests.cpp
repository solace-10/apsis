#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "components/orbital_elements_component.hpp"
#include "reference/sgp4_reference.hpp"
#include "space/sgp4.hpp"

using namespace WingsOfSteel;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{

const std::string kTleFile = std::string(APSIS_TEST_DATA_DIR) + "/SGP4-VER.TLE";

// The mean elements the reference was initialised from, taken back out of the initialised record.
//
// Initialisation does not disturb them: dpper's writes to the elements are guarded on
// init == 'n', and the t = 0 step sgp4init ends with works on copies. So these are still exactly
// what twoline2rv parsed, which is the whole point - feeding our initialisation the same doubles
// makes the comparison a measurement of the arithmetic rather than of the parsing, and keeps the
// component's float storage out of a test that is not about it.
SGP4Elements AsElements(const elsetrec& satrec)
{
    SGP4Elements elements;
    elements.bstar = satrec.bstar;
    elements.ecco = satrec.ecco;
    elements.argpo = satrec.argpo;
    elements.inclo = satrec.inclo;
    elements.mo = satrec.mo;
    elements.nodeo = satrec.nodeo;
    elements.noKozai = satrec.no_kozai;

    // The reference's own definition of the epoch it initialises from. JD 2433281.5 is
    // 1950 January 0.0.
    elements.epochDaysSince1950 = (satrec.jdsatepoch + satrec.jdsatepochF) - 2433281.5;

    return elements;
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

// The case the whole split rests on.
//
// Initialisation is the half of SGP4 that has to stay in double, so it is the half that was
// rewritten rather than shipped from Vallado's source - and a rewrite is worth exactly what its
// comparison against the original is worth. Every coefficient, over every near-earth element set
// in the published verification file, which between them exercise the branches that matter: a
// perigee low enough to move the atmospheric boundary, an eccentricity small enough to divide by,
// and inclinations either side of the retrograde singularity.
//
// Exact equality, deliberately. The expressions are written in the reference's own operation
// order for exactly this reason: there is no tolerance to argue about if the two agree bit for
// bit, and a difference in the last place would mean the arithmetic had been rearranged, which is
// worth being told about.
TEST_CASE("Our initialisation reproduces the reference's near-Earth coefficients", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    REQUIRE(cases.size() > 25);

    int nearEarthCases = 0;

    for (const Test::VerificationCase& verificationCase : cases)
    {
        const elsetrec& satrec = verificationCase.satrec;

        // The partition comes from the reference's own answer rather than from restating the
        // 225 minute threshold here, which would only check the test against itself.
        if (satrec.method != 'n')
        {
            continue;
        }

        ++nearEarthCases;
        INFO("satellite " << verificationCase.satnum);

        const SGP4ElementSet elementSet = SGP4Initialise(AsElements(satrec));

        auto check = [](const char* name, double ours, double theirs) {
            INFO("coefficient " << name);
            CHECK(ours == theirs);
        };

        CHECK(elementSet.method == SGP4Method::NearEarth);
        CHECK(elementSet.simplifiedDrag == (satrec.isimp == 1));

        // Carried through untouched, but the step reads them every call, so their being present
        // and right is as load bearing as any coefficient.
        check("bstar", elementSet.bstar, satrec.bstar);
        check("ecco", elementSet.ecco, satrec.ecco);
        check("inclo", elementSet.inclo, satrec.inclo);
        check("nodeo", elementSet.nodeo, satrec.nodeo);
        check("argpo", elementSet.argpo, satrec.argpo);
        check("mo", elementSet.mo, satrec.mo);

        check("no_unkozai", elementSet.no_unkozai, satrec.no_unkozai);
        check("aycof", elementSet.aycof, satrec.aycof);
        check("con41", elementSet.con41, satrec.con41);
        check("cc1", elementSet.cc1, satrec.cc1);
        check("cc4", elementSet.cc4, satrec.cc4);
        check("cc5", elementSet.cc5, satrec.cc5);
        check("d2", elementSet.d2, satrec.d2);
        check("d3", elementSet.d3, satrec.d3);
        check("d4", elementSet.d4, satrec.d4);
        check("delmo", elementSet.delmo, satrec.delmo);
        check("eta", elementSet.eta, satrec.eta);
        check("argpdot", elementSet.argpdot, satrec.argpdot);
        check("omgcof", elementSet.omgcof, satrec.omgcof);
        check("sinmao", elementSet.sinmao, satrec.sinmao);
        check("t2cof", elementSet.t2cof, satrec.t2cof);
        check("t3cof", elementSet.t3cof, satrec.t3cof);
        check("t4cof", elementSet.t4cof, satrec.t4cof);
        check("t5cof", elementSet.t5cof, satrec.t5cof);
        check("x1mth2", elementSet.x1mth2, satrec.x1mth2);
        check("x7thm1", elementSet.x7thm1, satrec.x7thm1);
        check("mdot", elementSet.mdot, satrec.mdot);
        check("nodedot", elementSet.nodedot, satrec.nodedot);
        check("xlcof", elementSet.xlcof, satrec.xlcof);
        check("xmcof", elementSet.xmcof, satrec.xmcof);
        check("nodecf", elementSet.nodecf, satrec.nodecf);
    }

    // Stated so that the case above cannot quietly stop covering anything: if the loop ever skips
    // everything, this is what says so rather than a green run over zero comparisons.
    CHECK(nearEarthCases == 9);
}

// Which algorithm an element set belongs to, and how much of the drag model it gets, are both
// decided at initialisation and both change what the step has to do. Neither is visible in a
// position, so neither is covered by the propagation cases.
TEST_CASE("The near-Earth and deep-space partition matches the reference", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    REQUIRE(cases.size() > 25);

    int deepSpaceCases = 0;

    for (const Test::VerificationCase& verificationCase : cases)
    {
        const elsetrec& satrec = verificationCase.satrec;
        INFO("satellite " << verificationCase.satnum);

        const SGP4ElementSet elementSet = SGP4Initialise(AsElements(satrec));
        const SGP4Method expected = satrec.method == 'd' ? SGP4Method::DeepSpace : SGP4Method::NearEarth;
        CHECK(elementSet.method == expected);

        if (expected == SGP4Method::DeepSpace)
        {
            ++deepSpaceCases;

            // Deep space stops at the partition rather than returning half a coefficient block.
            // The reference computes the near-earth coefficients for these orbits too, but they
            // are only part of what SDP4 then steps with, and a set that looks usable and is not
            // is worse than an obviously empty one. This is what says so.
            CHECK(elementSet.no_unkozai == 0.0);
            CHECK(elementSet.cc1 == 0.0);
            CHECK(elementSet.mdot == 0.0);
            CHECK(elementSet.aycof == 0.0);
        }
        else
        {
            // The simplified drag model, which drops the higher order terms below a 220 km
            // perigee. Deep space is excluded because the reference forces the flag on there, and
            // we never reach that decision.
            CHECK(elementSet.simplifiedDrag == (satrec.isimp == 1));
        }
    }

    CHECK(deepSpaceCases == 24);
}

// The one place the served record's units meet the algorithm's.
//
// Not circular: the expected values come from twoline2rv's own parse of the TLE text, and the
// component is built by converting back out of them, so what is being checked is that our
// conversion inverts Vallado's rather than that it agrees with itself.
TEST_CASE("An element set converts into the units SGP4 initialises from", "[space][sgp4]")
{
    // 06251 is a real low Earth orbit rather than one of the file's constructed edge cases, so
    // every field it exercises has a value a served record would plausibly carry.
    const elsetrec satrec = FindCase("06251").satrec;
    const OrbitalElementsComponent component = Test::AsComponent(satrec);

    const SGP4Elements elements = MakeSGP4Elements(component);

    // The component stores floats, so nothing here can be closer than about seven significant
    // digits however the conversion is written. That is the tolerance being expressed: an angle
    // in degrees loses at most 4.3e-5 of a degree, which is 7.5e-7 radians.
    CHECK_THAT(elements.inclo, WithinAbs(satrec.inclo, 1.0e-6));
    CHECK_THAT(elements.nodeo, WithinAbs(satrec.nodeo, 1.0e-6));
    CHECK_THAT(elements.argpo, WithinAbs(satrec.argpo, 1.0e-6));
    CHECK_THAT(elements.mo, WithinAbs(satrec.mo, 1.0e-6));

    CHECK_THAT(elements.noKozai, WithinRel(satrec.no_kozai, 1.0e-6));
    CHECK_THAT(elements.ecco, WithinRel(satrec.ecco, 1.0e-6));
    CHECK_THAT(elements.bstar, WithinRel(satrec.bstar, 1.0e-6));

    // The epoch is the conversion with no float in it - it goes through a text round trip printed
    // to microseconds, and system_clock carries microseconds - so it can be held to far more than
    // the rest. A thousandth of a second, in days.
    const double expectedEpoch = (satrec.jdsatepoch + satrec.jdsatepochF) - 2433281.5;
    CHECK_THAT(elements.epochDaysSince1950, WithinAbs(expectedEpoch, 1.0e-8));
}

// The partition decided on the numbers the game actually carries, rather than the reference's.
//
// `Sector::InitializeSpaceObjects` derives an element set from an `OrbitalElementsComponent` the
// moment an object is created, which is the float-and-degrees path rather than the doubles every
// case above feeds in. A geostationary orbit sits some twelve hundred minutes past the threshold,
// so nothing float precision could do to it would move it across - but this is the one place the
// two paths could part company, and it is the shape SGP4Component now depends on.
TEST_CASE("A geostationary element set reaches the deep-space branch through the component", "[space][sgp4]")
{
    // 25954, at 0.0004 degrees of inclination and 1.00271289 revolutions a day: a real
    // geostationary satellite rather than one of the file's constructed cases.
    const elsetrec satrec = FindCase("25954").satrec;
    REQUIRE(satrec.method == 'd');

    const OrbitalElementsComponent component = Test::AsComponent(satrec);
    const SGP4ElementSet elementSet = SGP4Initialise(MakeSGP4Elements(component));

    CHECK(elementSet.method == SGP4Method::DeepSpace);

    // Stopped at the partition rather than half initialised. Every geostationary object in the
    // catalogue now carries one of these, so what an unusable one looks like has to be obvious.
    CHECK(elementSet.no_unkozai == 0.0);
    CHECK(elementSet.cc1 == 0.0);
    CHECK(elementSet.mdot == 0.0);
    CHECK(elementSet.aycof == 0.0);
    CHECK(elementSet.simplifiedDrag == false);
}
