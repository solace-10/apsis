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
// comparison against the original is worth. Every coefficient, over every element set in the
// published verification file, which between them exercise the branches that matter: a perigee low
// enough to move the atmospheric boundary, an eccentricity small enough to divide by, inclinations
// either side of the retrograde singularity, and - on the deep-space side - both resonances, both
// arms of the eccentricity fits inside the half-day one, and inclinations near enough to zero and
// to pi to trip the polar guard.
//
// Exact equality, deliberately. The expressions are written in the reference's own operation
// order for exactly this reason: there is no tolerance to argue about if the two agree bit for
// bit, and a difference in the last place would mean the arithmetic had been rearranged, which is
// worth being told about.
TEST_CASE("Our initialisation reproduces the reference's coefficients", "[space][sgp4]")
{
    const std::vector<Test::VerificationCase> cases = Test::LoadVerificationCases(kTleFile);
    REQUIRE(cases.size() > 25);

    int nearEarthCases = 0;
    int deepSpaceCases = 0;

    for (const Test::VerificationCase& verificationCase : cases)
    {
        const elsetrec& satrec = verificationCase.satrec;

        // The partition comes from the reference's own answer rather than from restating the
        // 225 minute threshold here, which would only check the test against itself.
        const bool deepSpace = satrec.method == 'd';
        (deepSpace ? deepSpaceCases : nearEarthCases)++;

        INFO("satellite " << verificationCase.satnum);

        const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(satrec));

        auto check = [](const char* name, double ours, double theirs) {
            INFO("coefficient " << name);
            CHECK(ours == theirs);
        };

        CHECK(elementSet.method == (deepSpace ? SGP4Method::DeepSpace : SGP4Method::NearEarth));
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
        check("mdot", elementSet.mdot, satrec.mdot);
        check("nodedot", elementSet.nodedot, satrec.nodedot);
        check("xmcof", elementSet.xmcof, satrec.xmcof);
        check("nodecf", elementSet.nodecf, satrec.nodecf);

        // The five the deep-space step recomputes, and the reason they are checked here only for
        // the near-earth cases.
        //
        // All five are functions of the inclination, and the lunar-solar periodics move the
        // inclination - so on the deep-space branch the step works them out again from the
        // perturbed value rather than reading initialisation's. The reference does that by writing
        // them back into its element record, and its sgp4init ends by stepping to tsince = 0. So
        // for a deep-space satrec these fields hold what its STEP produced at the epoch, not what
        // its initialisation produced, and comparing ours against them would be comparing two
        // different quantities that happen to share a name. Initialisation's own values are no
        // longer observable in the reference at all.
        //
        // Nothing is lost by stopping here. Ours are locals inside the step rather than fields, so
        // if the recomputation were wrong every deep-space position would be wrong, and
        // sgp4_step_tests.cpp compares those against the reference bit for bit over every
        // published row.
        if (!deepSpace)
        {
            check("aycof", elementSet.aycof, satrec.aycof);
            check("con41", elementSet.con41, satrec.con41);
            check("x1mth2", elementSet.x1mth2, satrec.x1mth2);
            check("x7thm1", elementSet.x7thm1, satrec.x7thm1);
            check("xlcof", elementSet.xlcof, satrec.xlcof);
        }

        if (!deepSpace)
        {
            // Nothing should have been written into the deep-space half, and a near-earth object
            // carrying a stray coefficient there would be read by nothing and noticed by nobody.
            CHECK(elementSet.deepSpace.resonance == SDP4Resonance::None);
            CHECK(elementSet.deepSpace.gsto == 0.0);
            CHECK(elementSet.deepSpace.dedt == 0.0);
            CHECK(elementSet.deepSpace.zmos == 0.0);
            continue;
        }

        const SDP4Terms& terms = elementSet.deepSpace;

        CHECK(static_cast<int>(terms.resonance) == satrec.irez);

        // The five the reference stores and we do not. dscom sets them to zero and dpper only ever
        // subtracts them, so they cannot be anything else - but that is a claim about someone
        // else's code, and this is what checks it rather than believing it. See SDP4Terms.
        CHECK(satrec.peo == 0.0);
        CHECK(satrec.pgho == 0.0);
        CHECK(satrec.pho == 0.0);
        CHECK(satrec.pinco == 0.0);
        CHECK(satrec.plo == 0.0);

        check("gsto", terms.gsto, satrec.gsto);

        check("dedt", terms.dedt, satrec.dedt);
        check("didt", terms.didt, satrec.didt);
        check("dmdt", terms.dmdt, satrec.dmdt);
        check("dnodt", terms.dnodt, satrec.dnodt);
        check("domdt", terms.domdt, satrec.domdt);

        check("e3", terms.e3, satrec.e3);
        check("ee2", terms.ee2, satrec.ee2);
        check("se2", terms.se2, satrec.se2);
        check("se3", terms.se3, satrec.se3);
        check("sgh2", terms.sgh2, satrec.sgh2);
        check("sgh3", terms.sgh3, satrec.sgh3);
        check("sgh4", terms.sgh4, satrec.sgh4);
        check("sh2", terms.sh2, satrec.sh2);
        check("sh3", terms.sh3, satrec.sh3);
        check("si2", terms.si2, satrec.si2);
        check("si3", terms.si3, satrec.si3);
        check("sl2", terms.sl2, satrec.sl2);
        check("sl3", terms.sl3, satrec.sl3);
        check("sl4", terms.sl4, satrec.sl4);
        check("xgh2", terms.xgh2, satrec.xgh2);
        check("xgh3", terms.xgh3, satrec.xgh3);
        check("xgh4", terms.xgh4, satrec.xgh4);
        check("xh2", terms.xh2, satrec.xh2);
        check("xh3", terms.xh3, satrec.xh3);
        check("xi2", terms.xi2, satrec.xi2);
        check("xi3", terms.xi3, satrec.xi3);
        check("xl2", terms.xl2, satrec.xl2);
        check("xl3", terms.xl3, satrec.xl3);
        check("xl4", terms.xl4, satrec.xl4);
        check("zmol", terms.zmol, satrec.zmol);
        check("zmos", terms.zmos, satrec.zmos);

        check("d2201", terms.d2201, satrec.d2201);
        check("d2211", terms.d2211, satrec.d2211);
        check("d3210", terms.d3210, satrec.d3210);
        check("d3222", terms.d3222, satrec.d3222);
        check("d4410", terms.d4410, satrec.d4410);
        check("d4422", terms.d4422, satrec.d4422);
        check("d5220", terms.d5220, satrec.d5220);
        check("d5232", terms.d5232, satrec.d5232);
        check("d5421", terms.d5421, satrec.d5421);
        check("d5433", terms.d5433, satrec.d5433);
        check("del1", terms.del1, satrec.del1);
        check("del2", terms.del2, satrec.del2);
        check("del3", terms.del3, satrec.del3);

        check("xfact", terms.xfact, satrec.xfact);
        check("xlamo", terms.xlamo, satrec.xlamo);
    }

    // Stated so that the case above cannot quietly stop covering anything: if the loop ever skips
    // everything, this is what says so rather than a green run over zero comparisons.
    CHECK(nearEarthCases == 9);
    CHECK(deepSpaceCases == 24);
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

        const SGP4ElementSet elementSet = SGP4Initialise(Test::AsElements(satrec));
        const SGP4Method expected = satrec.method == 'd' ? SGP4Method::DeepSpace : SGP4Method::NearEarth;
        CHECK(elementSet.method == expected);

        // The simplified drag model, which drops the higher order terms below a 220 km perigee -
        // and which deep space takes whatever its perigee, because nothing up there decays over
        // the hours those terms describe. Both arms of that come from the reference's own flag.
        CHECK(elementSet.simplifiedDrag == (satrec.isimp == 1));

        if (expected == SGP4Method::DeepSpace)
        {
            ++deepSpaceCases;

            // Deep space no longer stops at the partition. It gets the whole near-earth block,
            // which SDP4 steps with as well, plus a block of its own on top - so what says an
            // element set is usable is that both halves are populated, where it used to be that
            // one half was empty. The full comparison is the case above; these are the fields that
            // would be zero if the deep-space branch had been skipped altogether.
            CHECK(elementSet.no_unkozai > 0.0);
            CHECK(elementSet.mdot > 0.0);
            CHECK(elementSet.deepSpace.gsto > 0.0);
            CHECK(elementSet.deepSpace.zmos != 0.0);
            CHECK(elementSet.simplifiedDrag);
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
//
// The resonance is the more interesting half. Unlike the deep-space partition it is decided on a
// narrow band - a mean motion within a fifth of one turn a day - and the mean motion is the field
// the component's float storage costs the most digits of, so this is the classification that could
// plausibly have gone differently on the two paths.
TEST_CASE("A geostationary element set reaches the deep-space branch through the component", "[space][sgp4]")
{
    // 25954, at 0.0004 degrees of inclination and 1.00271289 revolutions a day: a real
    // geostationary satellite rather than one of the file's constructed cases.
    const elsetrec satrec = FindCase("25954").satrec;
    REQUIRE(satrec.method == 'd');
    REQUIRE(satrec.irez == 1);

    const OrbitalElementsComponent component = Test::AsComponent(satrec);
    const SGP4ElementSet elementSet = SGP4Initialise(MakeSGP4Elements(component));

    CHECK(elementSet.method == SGP4Method::DeepSpace);
    CHECK(elementSet.deepSpace.resonance == SDP4Resonance::Synchronous);

    // Initialised rather than stopped at the partition. Every geostationary object in the
    // catalogue carries one of these and is now propagated from it, so a block that came back
    // empty would put the whole population at the origin.
    CHECK(elementSet.no_unkozai > 0.0);
    CHECK(elementSet.mdot > 0.0);
    CHECK(elementSet.deepSpace.gsto > 0.0);
    CHECK(elementSet.deepSpace.xlamo != 0.0);
    CHECK(elementSet.simplifiedDrag);

    // And it propagates to where a geostationary satellite is. Held loosely - the point is that
    // the float path produces an orbit rather than a number close to the reference's, which the
    // cases above already establish on the double path.
    const SGP4Position stepped = SGP4Step(elementSet, 1440.0);
    CHECK(stepped.error == SGP4Error::None);
    CHECK_THAT(glm::length(stepped.position), WithinRel(42164.0, 1.0e-3));
}
