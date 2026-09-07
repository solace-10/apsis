#include "space/sgp4.hpp"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "components/orbital_elements_component.hpp"

namespace WingsOfSteel
{

namespace
{

    // Appears throughout as the exponent relating mean motion to semi-major axis, and written out
    // rather than folded into the calls so that the division happens once and identically at each
    // use, which is what lets the result be compared against the reference exactly.
    constexpr double kTwoThirds = 2.0 / 3.0;

    // The period, in minutes, at or above which the element set belongs to SDP4 instead. Works out
    // at a semi-major axis of about 12250 km, so a circular orbit above roughly 5900 km - and any
    // orbit eccentric enough to take that long whatever its perigee.
    constexpr double kDeepSpacePeriodMinutes = 225.0;

    // Perigee below this altitude in km and the drag model drops its higher order terms.
    constexpr double kSimplifiedDragPerigee = 220.0;

    // Guards the division by (1 + cos i) in xlcof against an exactly retrograde orbit. The value is
    // the reference's, and it is a threshold on the divisor rather than on the inclination.
    constexpr double kRetrogradeSingularityGuard = 1.5e-12;

    // Revolutions per day to radians per minute, as a divisor: the reference divides by this
    // quantity rather than multiplying by its reciprocal, and the last bit of the mean motion is
    // worth keeping identical.
    const double kRevsPerDayToRadsPerMinute = 1440.0 / glm::two_pi<double>();

    // --- Deep space (SDP4) ---
    //
    // The four routines below are the reference's dscom, dsinit, dpper and dspace, written from
    // the papers under the names those papers use, exactly as the near-earth half above was. Two
    // of them run at initialisation and two at every step; the split is the same one the near
    // earth path uses, for the reasons set out at SDP4Terms in the header.
    //
    // The constants are the theories' own. Nothing here is derived from anything else here, so
    // they are stated in the order the routine that first needs them uses them rather than sorted.

    // The Sun's and Moon's mean motions in radians per minute, and their eccentricities. The
    // periodics are Fourier terms in these bodies' positions, which is the whole of what makes
    // deep space different: below 225 minutes their contribution averages away over an orbit.
    constexpr double kSolarMeanMotion = 1.19459e-5; // Vallado's zns
    constexpr double kSolarEccentricity = 0.01675; // zes
    constexpr double kLunarMeanMotion = 1.5835218e-4; // znl
    constexpr double kLunarEccentricity = 0.05490; // zel

    // The Sun's and Moon's contributions to the perturbing potential, and the orientation of the
    // Sun's orbit in the equatorial frame. The Moon's equivalents are not constants - its orbit
    // plane regresses with an 18.6 year period - so they are computed from the epoch in dscom.
    constexpr double kSolarPerturbation = 2.9864797e-6; // c1ss
    constexpr double kLunarPerturbation = 4.7968065e-7; // c1l
    constexpr double kSinSolarInclination = 0.39785416; // zsinis
    constexpr double kCosSolarInclination = 0.91744867; // zcosis
    constexpr double kCosSolarArgument = 0.1945905; // zcosgs
    constexpr double kSinSolarArgument = -0.98088458; // zsings

    // The Earth's rotation rate in radians per minute, which is what a resonance is in resonance
    // with. 7.29211514668855e-5 rad/s.
    constexpr double kEarthRotationRate = 4.37526908801129966e-3; // rptim

    // Amplitudes of the tesseral harmonics the two resonances run on, and the phase angles that go
    // with them.
    constexpr double kQ22 = 1.7891679e-6;
    constexpr double kQ31 = 2.1460748e-6;
    constexpr double kQ33 = 2.2123015e-7;
    constexpr double kRoot22 = 1.7891679e-6;
    constexpr double kRoot32 = 3.7393792e-7;
    constexpr double kRoot44 = 7.3636953e-9;
    constexpr double kRoot52 = 1.1428639e-7;
    constexpr double kRoot54 = 2.1765803e-9;
    constexpr double kFasx2 = 0.13130908;
    constexpr double kFasx4 = 2.8843198;
    constexpr double kFasx6 = 0.37448087;
    constexpr double kG22 = 5.7686396;
    constexpr double kG32 = 0.95240898;
    constexpr double kG44 = 1.8014998;
    constexpr double kG52 = 1.0508330;
    constexpr double kG54 = 4.4108898;

    // The resonance integrator's step, in minutes, and the second order coefficient that goes with
    // it. Euler-Maclaurin, so the step appears squared and halved as well as plain.
    constexpr double kResonanceStepMinutes = 720.0;
    constexpr double kResonanceHalfStepSquared = 259200.0; // 720^2 / 2

    // How many of those steps the integration is allowed before it gives up. The reference has no
    // such limit; this one exists because sgp4.wgsl cannot have an unbounded loop, and a bound the
    // shader keeps and this does not would leave the two no longer comparable - which is the whole
    // basis of sgp4_shader_tests.cpp.
    //
    // The number comes from how stale an element set can actually be. spacetrack.py queries
    // EPOCH/>now-30, so nothing enters the catalogue with an epoch over 30 days old, and
    // clear_stale_objects() removes anything not refreshed for 3 days - so |t| stays inside about
    // 33 days, or 47,520 minutes, which is 66 steps. 72 leaves a little margin over that.
    //
    // No verification case comes close, so this changes nothing the tests compare: across every
    // case in SGP4-VER.TLE at every published time, the integrator's worst is 14 steps, at
    // |t| = 9,360 minutes on 26900, and it averages 2.3.
    //
    // Reaching the limit means an element set so far from its epoch that the ingestion should
    // already have dropped it. Refusing it is better than the alternative, which is not a smaller
    // error but a wild one: ft would be the whole unintegrated remainder, and nm is quadratic in
    // it. Mirrored in sgp4.wgsl as kResonanceMaxSteps.
    constexpr int kResonanceMaxSteps = 72;

    // An inclination this close to zero or to pi sends the node's periodic through a division by
    // sin i. The reference drops the term rather than letting it blow up; 5.2359877e-2 rad is 3
    // degrees.
    constexpr double kPolarSingularityGuard = 5.2359877e-2;

    // Greenwich mean sidereal time at an element set's epoch, in radians.
    //
    // Deliberately NOT CalculateGMST() in earth_frame.hpp, which answers the same question to two
    // terms. That one is right for turning the Earth under the camera, where a hundredth of a
    // degree is invisible. This one is the phase a resonance is measured against: it feeds xlamo,
    // which the integrator then carries forward for months, so the quadratic and cubic terms are
    // the difference between agreeing with the reference and drifting away from it. The two are
    // meant to differ, and unifying them would break this one.
    double SiderealTimeAtEpoch(double epochDaysSince1950)
    {
        // 1950 January 0.0, the day count the algorithm's epoch is measured from, as a Julian date.
        constexpr double kJulianDate1950 = 2433281.5;
        constexpr double kJulianDateJ2000 = 2451545.0;
        constexpr double kDaysPerJulianCentury = 36525.0;

        const double julianDate = epochDaysSince1950 + kJulianDate1950;
        const double t = (julianDate - kJulianDateJ2000) / kDaysPerJulianCentury;

        // Seconds of time. The linear term is a full turn a sidereal day; the rest is the slow
        // change in the length of the day and in the precession of the equinox.
        const double seconds = -6.2e-6 * t * t * t + 0.093104 * t * t
            + (876600.0 * 3600.0 + 8640184.812866) * t + 67310.54841;

        // 360 degrees in 86400 seconds, so a second of time is a 240th of a degree.
        const double degreesToRadians = glm::pi<double>() / 180.0;
        double gsto = std::fmod(seconds * degreesToRadians / 240.0, glm::two_pi<double>());
        if (gsto < 0.0)
        {
            gsto += glm::two_pi<double>();
        }

        return gsto;
    }

    // What dscom works out that dsinit needs and nobody stores.
    //
    // dscom produces two kinds of thing: the periodic amplitudes, which the step reads at every
    // call and which therefore live in SDP4Terms, and these, which exist only long enough for
    // dsinit to turn them into secular rates. The reference passes all of it through one argument
    // list of eighty-odd references; separating the two by where they go is the whole difference.
    //
    // The s-series are the Moon's and the ss-series the Sun's, both being the amplitudes of that
    // body's disturbing function. The z-series are the direction cosines between the two orbit
    // planes, and likewise sz for the Sun.
    struct DeepSpaceCommonTerms
    {
        double sinim{ 0.0 };
        double cosim{ 0.0 };
        double emsq{ 0.0 };

        double s1{ 0.0 };
        double s2{ 0.0 };
        double s3{ 0.0 };
        double s4{ 0.0 };
        double s5{ 0.0 };

        double ss1{ 0.0 };
        double ss2{ 0.0 };
        double ss3{ 0.0 };
        double ss4{ 0.0 };
        double ss5{ 0.0 };

        double sz1{ 0.0 };
        double sz3{ 0.0 };
        double sz11{ 0.0 };
        double sz13{ 0.0 };
        double sz21{ 0.0 };
        double sz23{ 0.0 };
        double sz31{ 0.0 };
        double sz33{ 0.0 };

        double z1{ 0.0 };
        double z3{ 0.0 };
        double z11{ 0.0 };
        double z13{ 0.0 };
        double z21{ 0.0 };
        double z23{ 0.0 };
        double z31{ 0.0 };
        double z33{ 0.0 };
    };

    // The Sun's and Moon's disturbing functions, resolved into the orbit's own frame (dscom).
    //
    // Runs the same block of arithmetic twice, once per body: the solar pass uses the constants
    // above, and the lunar pass the orbit worked out from the epoch at the top. What differs
    // between the two is only where the body is and how strongly it pulls, which is why the
    // reference writes it as a two-iteration loop rather than twice, and why this does too.
    //
    // Writes the periodic amplitudes straight into the block the step will read, and returns the
    // rest for dsinit.
    DeepSpaceCommonTerms DeepSpaceCommon(const SGP4Elements& elements, double no_unkozai, SDP4Terms& terms)
    {
        const double twopi = glm::two_pi<double>();

        DeepSpaceCommonTerms common;

        const double em = elements.ecco;
        common.emsq = em * em;
        const double betasq = 1.0 - common.emsq;
        const double rtemsq = std::sqrt(betasq);

        const double snodm = std::sin(elements.nodeo);
        const double cnodm = std::cos(elements.nodeo);
        const double sinomm = std::sin(elements.argpo);
        const double cosomm = std::cos(elements.argpo);
        common.sinim = std::sin(elements.inclo);
        common.cosim = std::cos(elements.inclo);

        // Days since 1900, which is the epoch the lunar series below are stated from. The
        // reference also adds tc/1440, the offset from the element set's epoch that dscom is being
        // run at; ours is always zero, because we only ever initialise at the epoch itself.
        const double day = elements.epochDaysSince1950 + 18261.5;

        // Where the Moon's orbit plane is now. Its node regresses once every 18.6 years and its
        // inclination to the equator swings between 18 and 28 degrees as it does, so unlike the
        // Sun's these cannot be constants.
        const double xnodce = std::fmod(4.5236020 - 9.2422029e-4 * day, twopi);
        const double stem = std::sin(xnodce);
        const double ctem = std::cos(xnodce);
        const double zcosil = 0.91375164 - 0.03568096 * ctem;
        const double zsinil = std::sqrt(1.0 - zcosil * zcosil);
        const double zsinhl = 0.089683511 * stem / zsinil;
        const double zcoshl = std::sqrt(1.0 - zsinhl * zsinhl);
        const double gam = 5.8351514 + 0.0019443680 * day;
        double zx = 0.39785416 * stem / zsinil;
        const double zy = zcoshl * ctem + 0.91744867 * zsinhl * stem;
        zx = std::atan2(zx, zy);
        zx = gam + zx - xnodce;
        const double zcosgl = std::cos(zx);
        const double zsingl = std::sin(zx);

        double zcosg = kCosSolarArgument;
        double zsing = kSinSolarArgument;
        double zcosi = kCosSolarInclination;
        double zsini = kSinSolarInclination;
        double zcosh = cnodm;
        double zsinh = snodm;
        double cc = kSolarPerturbation;
        const double xnoi = 1.0 / no_unkozai;

        double s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0, s5 = 0.0, s6 = 0.0, s7 = 0.0;
        double z1 = 0.0, z2 = 0.0, z3 = 0.0;
        double z11 = 0.0, z12 = 0.0, z13 = 0.0;
        double z21 = 0.0, z22 = 0.0, z23 = 0.0;
        double z31 = 0.0, z32 = 0.0, z33 = 0.0;
        double ss1 = 0.0, ss2 = 0.0, ss3 = 0.0, ss4 = 0.0, ss5 = 0.0, ss6 = 0.0, ss7 = 0.0;
        double sz1 = 0.0, sz2 = 0.0, sz3 = 0.0;
        double sz11 = 0.0, sz12 = 0.0, sz13 = 0.0;
        double sz21 = 0.0, sz22 = 0.0, sz23 = 0.0;
        double sz31 = 0.0, sz32 = 0.0, sz33 = 0.0;

        for (int lsflg = 1; lsflg <= 2; lsflg++)
        {
            const double a1 = zcosg * zcosh + zsing * zcosi * zsinh;
            const double a3 = -zsing * zcosh + zcosg * zcosi * zsinh;
            const double a7 = -zcosg * zsinh + zsing * zcosi * zcosh;
            const double a8 = zsing * zsini;
            const double a9 = zsing * zsinh + zcosg * zcosi * zcosh;
            const double a10 = zcosg * zsini;
            const double a2 = common.cosim * a7 + common.sinim * a8;
            const double a4 = common.cosim * a9 + common.sinim * a10;
            const double a5 = -common.sinim * a7 + common.cosim * a8;
            const double a6 = -common.sinim * a9 + common.cosim * a10;

            const double x1 = a1 * cosomm + a2 * sinomm;
            const double x2 = a3 * cosomm + a4 * sinomm;
            const double x3 = -a1 * sinomm + a2 * cosomm;
            const double x4 = -a3 * sinomm + a4 * cosomm;
            const double x5 = a5 * sinomm;
            const double x6 = a6 * sinomm;
            const double x7 = a5 * cosomm;
            const double x8 = a6 * cosomm;

            z31 = 12.0 * x1 * x1 - 3.0 * x3 * x3;
            z32 = 24.0 * x1 * x2 - 6.0 * x3 * x4;
            z33 = 12.0 * x2 * x2 - 3.0 * x4 * x4;
            z1 = 3.0 * (a1 * a1 + a2 * a2) + z31 * common.emsq;
            z2 = 6.0 * (a1 * a3 + a2 * a4) + z32 * common.emsq;
            z3 = 3.0 * (a3 * a3 + a4 * a4) + z33 * common.emsq;
            z11 = -6.0 * a1 * a5 + common.emsq * (-24.0 * x1 * x7 - 6.0 * x3 * x5);
            z12 = -6.0 * (a1 * a6 + a3 * a5) + common.emsq * (-24.0 * (x2 * x7 + x1 * x8) - 6.0 * (x3 * x6 + x4 * x5));
            z13 = -6.0 * a3 * a6 + common.emsq * (-24.0 * x2 * x8 - 6.0 * x4 * x6);
            z21 = 6.0 * a2 * a5 + common.emsq * (24.0 * x1 * x5 - 6.0 * x3 * x7);
            z22 = 6.0 * (a4 * a5 + a2 * a6) + common.emsq * (24.0 * (x2 * x5 + x1 * x6) - 6.0 * (x4 * x7 + x3 * x8));
            z23 = 6.0 * a4 * a6 + common.emsq * (24.0 * x2 * x6 - 6.0 * x4 * x8);
            z1 = z1 + z1 + betasq * z31;
            z2 = z2 + z2 + betasq * z32;
            z3 = z3 + z3 + betasq * z33;
            s3 = cc * xnoi;
            s2 = -0.5 * s3 / rtemsq;
            s4 = s3 * rtemsq;
            s1 = -15.0 * em * s4;
            s5 = x1 * x3 + x2 * x4;
            s6 = x2 * x3 + x1 * x4;
            s7 = x2 * x4 - x1 * x3;

            // End of the solar pass: put it aside and swap in the Moon for the second turn.
            if (lsflg == 1)
            {
                ss1 = s1;
                ss2 = s2;
                ss3 = s3;
                ss4 = s4;
                ss5 = s5;
                ss6 = s6;
                ss7 = s7;
                sz1 = z1;
                sz2 = z2;
                sz3 = z3;
                sz11 = z11;
                sz12 = z12;
                sz13 = z13;
                sz21 = z21;
                sz22 = z22;
                sz23 = z23;
                sz31 = z31;
                sz32 = z32;
                sz33 = z33;
                zcosg = zcosgl;
                zsing = zsingl;
                zcosi = zcosil;
                zsini = zsinil;
                zcosh = zcoshl * cnodm + zsinhl * snodm;
                zsinh = snodm * zcoshl - cnodm * zsinhl;
                cc = kLunarPerturbation;
            }
        }

        // Where the two bodies are at the epoch, which is the phase the step advances.
        terms.zmol = std::fmod(4.7199672 + 0.22997150 * day - gam, twopi);
        terms.zmos = std::fmod(6.2565837 + 0.017201977 * day, twopi);

        // The solar periodics.
        terms.se2 = 2.0 * ss1 * ss6;
        terms.se3 = 2.0 * ss1 * ss7;
        terms.si2 = 2.0 * ss2 * sz12;
        terms.si3 = 2.0 * ss2 * (sz13 - sz11);
        terms.sl2 = -2.0 * ss3 * sz2;
        terms.sl3 = -2.0 * ss3 * (sz3 - sz1);
        terms.sl4 = -2.0 * ss3 * (-21.0 - 9.0 * common.emsq) * kSolarEccentricity;
        terms.sgh2 = 2.0 * ss4 * sz32;
        terms.sgh3 = 2.0 * ss4 * (sz33 - sz31);
        terms.sgh4 = -18.0 * ss4 * kSolarEccentricity;
        terms.sh2 = -2.0 * ss2 * sz22;
        terms.sh3 = -2.0 * ss2 * (sz23 - sz21);

        // The lunar periodics.
        terms.ee2 = 2.0 * s1 * s6;
        terms.e3 = 2.0 * s1 * s7;
        terms.xi2 = 2.0 * s2 * z12;
        terms.xi3 = 2.0 * s2 * (z13 - z11);
        terms.xl2 = -2.0 * s3 * z2;
        terms.xl3 = -2.0 * s3 * (z3 - z1);
        terms.xl4 = -2.0 * s3 * (-21.0 - 9.0 * common.emsq) * kLunarEccentricity;
        terms.xgh2 = 2.0 * s4 * z32;
        terms.xgh3 = 2.0 * s4 * (z33 - z31);
        terms.xgh4 = -18.0 * s4 * kLunarEccentricity;
        terms.xh2 = -2.0 * s2 * z22;
        terms.xh3 = -2.0 * s2 * (z23 - z21);

        common.s1 = s1;
        common.s2 = s2;
        common.s3 = s3;
        common.s4 = s4;
        common.s5 = s5;
        common.ss1 = ss1;
        common.ss2 = ss2;
        common.ss3 = ss3;
        common.ss4 = ss4;
        common.ss5 = ss5;
        common.sz1 = sz1;
        common.sz3 = sz3;
        common.sz11 = sz11;
        common.sz13 = sz13;
        common.sz21 = sz21;
        common.sz23 = sz23;
        common.sz31 = sz31;
        common.sz33 = sz33;
        common.z1 = z1;
        common.z3 = z3;
        common.z11 = z11;
        common.z13 = z13;
        common.z21 = z21;
        common.z23 = z23;
        common.z31 = z31;
        common.z33 = z33;

        return common;
    }

    // Turns the disturbing functions above into the rates and resonance terms the step reads
    // (dsinit).
    //
    // Two unrelated jobs, which the reference does in one routine and which are kept together here
    // for the same reason: the first needs everything dscom produced, and the second needs the
    // first's answer. The first is the secular part - how fast the Sun and Moon between them drag
    // each mean element - and applies to every deep-space orbit. The second only applies to the
    // few in resonance with the Earth's own shape.
    //
    // The reference also advances the mean elements by the secular rates here, and initialises its
    // integrator's position. Both are omitted: it does that at tsince = 0, where every rate is
    // multiplied by zero, and the integrator is not state we keep. See SDP4Terms.
    void DeepSpaceInitialise(const SGP4Elements& elements, double no_unkozai, double mdot, double nodedot, double xpidot, const DeepSpaceCommonTerms& common, SDP4Terms& terms)
    {
        const double twopi = glm::two_pi<double>();
        const double pi = glm::pi<double>();

        // Which resonance, if any. Synchronous is a mean motion within a fifth of one turn a day;
        // half-day is two turns a day, and only counts if the orbit is eccentric enough for the
        // harmonics to bite at perigee.
        terms.resonance = SDP4Resonance::None;
        if ((no_unkozai < 0.0052359877) && (no_unkozai > 0.0034906585))
        {
            terms.resonance = SDP4Resonance::Synchronous;
        }
        if ((no_unkozai >= 8.26e-3) && (no_unkozai <= 9.24e-3) && (elements.ecco >= 0.5))
        {
            terms.resonance = SDP4Resonance::HalfDay;
        }

        // --- The secular rates, which every deep-space orbit gets ---

        const bool nearPolarSingularity = (elements.inclo < kPolarSingularityGuard) || (elements.inclo > pi - kPolarSingularityGuard);

        const double ses = common.ss1 * kSolarMeanMotion * common.ss5;
        const double sis = common.ss2 * kSolarMeanMotion * (common.sz11 + common.sz13);
        const double sls = -kSolarMeanMotion * common.ss3 * (common.sz1 + common.sz3 - 14.0 - 6.0 * common.emsq);
        const double sghs = common.ss4 * kSolarMeanMotion * (common.sz31 + common.sz33 - 6.0);
        double shs = -kSolarMeanMotion * common.ss2 * (common.sz21 + common.sz23);
        if (nearPolarSingularity)
        {
            shs = 0.0;
        }
        if (common.sinim != 0.0)
        {
            shs = shs / common.sinim;
        }
        const double sgs = sghs - common.cosim * shs;

        terms.dedt = ses + common.s1 * kLunarMeanMotion * common.s5;
        terms.didt = sis + common.s2 * kLunarMeanMotion * (common.z11 + common.z13);
        terms.dmdt = sls - kLunarMeanMotion * common.s3 * (common.z1 + common.z3 - 14.0 - 6.0 * common.emsq);
        const double sghl = common.s4 * kLunarMeanMotion * (common.z31 + common.z33 - 6.0);
        double shll = -kLunarMeanMotion * common.s2 * (common.z21 + common.z23);
        if (nearPolarSingularity)
        {
            shll = 0.0;
        }
        terms.domdt = sgs + sghl;
        terms.dnodt = shs;
        if (common.sinim != 0.0)
        {
            terms.domdt = terms.domdt - common.cosim / common.sinim * shll;
            terms.dnodt = terms.dnodt + shll / common.sinim;
        }

        if (terms.resonance == SDP4Resonance::None)
        {
            return;
        }

        // --- The resonance terms, for the few orbits that have one ---

        // Sidereal time at the epoch. The reference offsets it by tc, which is zero here for the
        // same reason it is in dscom.
        const double theta = std::fmod(terms.gsto, twopi);

        const double aonv = std::pow(no_unkozai / kSGP4Xke, kTwoThirds);

        if (terms.resonance == SDP4Resonance::HalfDay)
        {
            // The eccentricity dependence of the 2:1 harmonics, as piecewise polynomial fits. The
            // reference swaps the eccentricity dscom returned for the one initialisation was given
            // around this block; at the epoch they are the same number, so this reads the elements
            // directly and the swap disappears.
            const double em = elements.ecco;
            const double emsq = common.emsq;
            const double eoc = em * emsq;

            const double g201 = -0.306 - (em - 0.64) * 0.440;

            double g211, g310, g322, g410, g422, g520;
            if (em <= 0.65)
            {
                g211 = 3.616 - 13.2470 * em + 16.2900 * emsq;
                g310 = -19.302 + 117.3900 * em - 228.4190 * emsq + 156.5910 * eoc;
                g322 = -18.9068 + 109.7927 * em - 214.6334 * emsq + 146.5816 * eoc;
                g410 = -41.122 + 242.6940 * em - 471.0940 * emsq + 313.9530 * eoc;
                g422 = -146.407 + 841.8800 * em - 1629.014 * emsq + 1083.4350 * eoc;
                g520 = -532.114 + 3017.977 * em - 5740.032 * emsq + 3708.2760 * eoc;
            }
            else
            {
                g211 = -72.099 + 331.819 * em - 508.738 * emsq + 266.724 * eoc;
                g310 = -346.844 + 1582.851 * em - 2415.925 * emsq + 1246.113 * eoc;
                g322 = -342.585 + 1554.908 * em - 2366.899 * emsq + 1215.972 * eoc;
                g410 = -1052.797 + 4758.686 * em - 7193.992 * emsq + 3651.957 * eoc;
                g422 = -3581.690 + 16178.110 * em - 24462.770 * emsq + 12422.520 * eoc;
                if (em > 0.715)
                {
                    g520 = -5149.66 + 29936.92 * em - 54087.36 * emsq + 31324.56 * eoc;
                }
                else
                {
                    g520 = 1464.74 - 4664.75 * em + 3763.64 * emsq;
                }
            }

            double g533, g521, g532;
            if (em < 0.7)
            {
                g533 = -919.22770 + 4988.6100 * em - 9064.7700 * emsq + 5542.21 * eoc;
                g521 = -822.71072 + 4568.6173 * em - 8491.4146 * emsq + 5337.524 * eoc;
                g532 = -853.66600 + 4690.2500 * em - 8624.7700 * emsq + 5341.4 * eoc;
            }
            else
            {
                g533 = -37995.780 + 161616.52 * em - 229838.20 * emsq + 109377.94 * eoc;
                g521 = -51752.104 + 218913.95 * em - 309468.16 * emsq + 146349.42 * eoc;
                g532 = -40023.880 + 170470.89 * em - 242699.48 * emsq + 115605.82 * eoc;
            }

            // The inclination dependence.
            const double cosisq = common.cosim * common.cosim;
            const double sini2 = common.sinim * common.sinim;
            const double f220 = 0.75 * (1.0 + 2.0 * common.cosim + cosisq);
            const double f221 = 1.5 * sini2;
            const double f321 = 1.875 * common.sinim * (1.0 - 2.0 * common.cosim - 3.0 * cosisq);
            const double f322 = -1.875 * common.sinim * (1.0 + 2.0 * common.cosim - 3.0 * cosisq);
            const double f441 = 35.0 * sini2 * f220;
            const double f442 = 39.3750 * sini2 * sini2;
            const double f522 = 9.84375 * common.sinim * (sini2 * (1.0 - 2.0 * common.cosim - 5.0 * cosisq) + 0.33333333 * (-2.0 + 4.0 * common.cosim + 6.0 * cosisq));
            const double f523 = common.sinim * (4.92187512 * sini2 * (-2.0 - 4.0 * common.cosim + 10.0 * cosisq) + 6.56250012 * (1.0 + 2.0 * common.cosim - 3.0 * cosisq));
            const double f542 = 29.53125 * common.sinim * (2.0 - 8.0 * common.cosim + cosisq * (-12.0 + 8.0 * common.cosim + 10.0 * cosisq));
            const double f543 = 29.53125 * common.sinim * (-2.0 - 8.0 * common.cosim + cosisq * (12.0 + 8.0 * common.cosim - 10.0 * cosisq));

            const double xno2 = no_unkozai * no_unkozai;
            const double ainv2 = aonv * aonv;
            double temp1 = 3.0 * xno2 * ainv2;
            double temp = temp1 * kRoot22;
            terms.d2201 = temp * f220 * g201;
            terms.d2211 = temp * f221 * g211;
            temp1 = temp1 * aonv;
            temp = temp1 * kRoot32;
            terms.d3210 = temp * f321 * g310;
            terms.d3222 = temp * f322 * g322;
            temp1 = temp1 * aonv;
            temp = 2.0 * temp1 * kRoot44;
            terms.d4410 = temp * f441 * g410;
            terms.d4422 = temp * f442 * g422;
            temp1 = temp1 * aonv;
            temp = temp1 * kRoot52;
            terms.d5220 = temp * f522 * g520;
            terms.d5232 = temp * f523 * g532;
            temp = 2.0 * temp1 * kRoot54;
            terms.d5421 = temp * f542 * g521;
            terms.d5433 = temp * f543 * g533;

            terms.xlamo = std::fmod(elements.mo + elements.nodeo + elements.nodeo - theta - theta, twopi);
            terms.xfact = mdot + terms.dmdt + 2.0 * (nodedot + terms.dnodt - kEarthRotationRate) - no_unkozai;
        }
        else
        {
            const double g200 = 1.0 + common.emsq * (-2.5 + 0.8125 * common.emsq);
            const double g310 = 1.0 + 2.0 * common.emsq;
            const double g300 = 1.0 + common.emsq * (-6.0 + 6.60937 * common.emsq);
            const double f220 = 0.75 * (1.0 + common.cosim) * (1.0 + common.cosim);
            const double f311 = 0.9375 * common.sinim * common.sinim * (1.0 + 3.0 * common.cosim) - 0.75 * (1.0 + common.cosim);
            double f330 = 1.0 + common.cosim;
            f330 = 1.875 * f330 * f330 * f330;

            terms.del1 = 3.0 * no_unkozai * no_unkozai * aonv * aonv;
            terms.del2 = 2.0 * terms.del1 * f220 * g200 * kQ22;
            terms.del3 = 3.0 * terms.del1 * f330 * g300 * kQ33 * aonv;
            terms.del1 = terms.del1 * f311 * g310 * kQ31 * aonv;

            terms.xlamo = std::fmod(elements.mo + elements.nodeo + elements.argpo - theta, twopi);
            terms.xfact = mdot + xpidot - kEarthRotationRate + terms.dmdt + terms.domdt + terms.dnodt - no_unkozai;
        }
    }

    // The lunar-solar periodics, applied to one set of osculating elements at one time (dpper).
    //
    // Periodic rather than secular: everything here is a Fourier term in where the Sun and Moon
    // are at that moment, so it is recomputed from scratch at every call and never accumulates.
    // That is what makes the deep-space step as stateless as the near-earth one.
    //
    // The reference also calls this once during initialisation, with an init flag that skips every
    // write it makes - so that call changes nothing, and there is no equivalent of it here. What
    // it would have subtracted is the peo family, which is identically zero; see SDP4Terms.
    void DeepSpacePeriodics(const SDP4Terms& terms, double t, double& ep, double& inclp, double& nodep, double& argpp, double& mp)
    {
        const double twopi = glm::two_pi<double>();

        // The Sun, advanced from the epoch to now, and its equation of the centre to first order.
        double zm = terms.zmos + kSolarMeanMotion * t;
        double zf = zm + 2.0 * kSolarEccentricity * std::sin(zm);
        double sinzf = std::sin(zf);
        double f2 = 0.5 * sinzf * sinzf - 0.25;
        double f3 = -0.5 * sinzf * std::cos(zf);
        const double ses = terms.se2 * f2 + terms.se3 * f3;
        const double sis = terms.si2 * f2 + terms.si3 * f3;
        const double sls = terms.sl2 * f2 + terms.sl3 * f3 + terms.sl4 * sinzf;
        const double sghs = terms.sgh2 * f2 + terms.sgh3 * f3 + terms.sgh4 * sinzf;
        const double shs = terms.sh2 * f2 + terms.sh3 * f3;

        // And the Moon.
        zm = terms.zmol + kLunarMeanMotion * t;
        zf = zm + 2.0 * kLunarEccentricity * std::sin(zm);
        sinzf = std::sin(zf);
        f2 = 0.5 * sinzf * sinzf - 0.25;
        f3 = -0.5 * sinzf * std::cos(zf);
        const double sel = terms.ee2 * f2 + terms.e3 * f3;
        const double sil = terms.xi2 * f2 + terms.xi3 * f3;
        const double sll = terms.xl2 * f2 + terms.xl3 * f3 + terms.xl4 * sinzf;
        const double sghl = terms.xgh2 * f2 + terms.xgh3 * f3 + terms.xgh4 * sinzf;
        const double shll = terms.xh2 * f2 + terms.xh3 * f3;

        const double pe = ses + sel;
        double pinc = sis + sil;
        const double pl = sls + sll;
        double pgh = sghs + sghl;
        double ph = shs + shll;

        inclp = inclp + pinc;
        ep = ep + pe;
        const double sinip = std::sin(inclp);
        const double cosip = std::cos(inclp);

        // Two ways of applying the same corrections. Above about eleven and a half degrees the
        // node and the argument of perigee are well enough separated to be corrected directly;
        // below it they are not, and Lyddane's formulation - which corrects the combination that
        // stays well defined as the inclination goes to zero - is used instead. The threshold is
        // the reference's, and the inclination tested is the perturbed one rather than the
        // original, which is the choice it documents as GSFC's.
        if (inclp >= 0.2)
        {
            ph = ph / sinip;
            pgh = pgh - cosip * ph;
            argpp = argpp + pgh;
            nodep = nodep + ph;
            mp = mp + pl;
        }
        else
        {
            const double sinop = std::sin(nodep);
            const double cosop = std::cos(nodep);
            double alfdp = sinip * sinop;
            double betdp = sinip * cosop;
            const double dalf = ph * cosop + pinc * cosip * sinop;
            const double dbet = -ph * sinop + pinc * cosip * cosop;
            alfdp = alfdp + dalf;
            betdp = betdp + dbet;
            nodep = std::fmod(nodep, twopi);

            double xls = mp + argpp + cosip * nodep;
            const double dls = pl + pgh - pinc * nodep * sinip;
            xls = xls + dls;
            const double xnoh = nodep;
            nodep = std::atan2(alfdp, betdp);

            // Keeping the node on the same turn it was on before atan2 put it back in range.
            if (std::fabs(xnoh - nodep) > glm::pi<double>())
            {
                nodep = nodep < xnoh ? nodep + twopi : nodep - twopi;
            }

            mp = mp + pl;
            argpp = xls - mp - cosip * nodep;
        }
    }

    // The secular contribution of the Sun, the Moon and - for a resonant orbit - the Earth's own
    // shape (dspace).
    //
    // The first part is a straight multiplication by elapsed time. The second is an integration,
    // because a resonance's whole character is that its effect on the mean motion depends on where
    // the object already is, so it cannot be written down in closed form. That integration is
    // restarted from the epoch at every call rather than continued from the last one; SDP4Terms
    // explains why that is both correct and affordable.
    // False when the integration could not reach t inside kResonanceMaxSteps, which is the caller's
    // to turn into an error - the same division of labour DeepSpacePeriodics() and Vallado's error
    // 3 already use.
    bool DeepSpaceSecular(const SGP4ElementSet& elementSet, double t, double& em, double& inclm, double& argpm, double& nodem, double& mm, double& nm)
    {
        const SDP4Terms& terms = elementSet.deepSpace;
        const double twopi = glm::two_pi<double>();

        const double theta = std::fmod(terms.gsto + t * kEarthRotationRate, twopi);

        em = em + terms.dedt * t;
        inclm = inclm + terms.didt * t;
        argpm = argpm + terms.domdt * t;
        nodem = nodem + terms.dnodt * t;
        mm = mm + terms.dmdt * t;

        if (terms.resonance == SDP4Resonance::None)
        {
            return true;
        }

        // Euler-Maclaurin, in whole steps of twelve hours towards the time asked for and then a
        // partial one to land on it. The reference resumes this from wherever the last call left
        // it; starting from the epoch every time visits the same states in the same order, so it
        // is the same arithmetic, and it is what lets this function take no state.
        double atime = 0.0;
        double xli = terms.xlamo;
        double xni = elementSet.no_unkozai;
        const double delt = t > 0.0 ? kResonanceStepMinutes : -kResonanceStepMinutes;

        double xndt = 0.0;
        double xldot = 0.0;
        double xnddt = 0.0;
        double ft = 0.0;

        bool reached = false;
        for (int step = 0; step <= kResonanceMaxSteps; step++)
        {
            if (terms.resonance == SDP4Resonance::Synchronous)
            {
                xndt = terms.del1 * std::sin(xli - kFasx2) + terms.del2 * std::sin(2.0 * (xli - kFasx4)) + terms.del3 * std::sin(3.0 * (xli - kFasx6));
                xldot = xni + terms.xfact;
                xnddt = terms.del1 * std::cos(xli - kFasx2) + 2.0 * terms.del2 * std::cos(2.0 * (xli - kFasx4)) + 3.0 * terms.del3 * std::cos(3.0 * (xli - kFasx6));
                xnddt = xnddt * xldot;
            }
            else
            {
                const double xomi = elementSet.argpo + elementSet.argpdot * atime;
                const double x2omi = xomi + xomi;
                const double x2li = xli + xli;
                xndt = terms.d2201 * std::sin(x2omi + xli - kG22) + terms.d2211 * std::sin(xli - kG22)
                    + terms.d3210 * std::sin(xomi + xli - kG32) + terms.d3222 * std::sin(-xomi + xli - kG32)
                    + terms.d4410 * std::sin(x2omi + x2li - kG44) + terms.d4422 * std::sin(x2li - kG44)
                    + terms.d5220 * std::sin(xomi + xli - kG52) + terms.d5232 * std::sin(-xomi + xli - kG52)
                    + terms.d5421 * std::sin(xomi + x2li - kG54) + terms.d5433 * std::sin(-xomi + x2li - kG54);
                xldot = xni + terms.xfact;
                xnddt = terms.d2201 * std::cos(x2omi + xli - kG22) + terms.d2211 * std::cos(xli - kG22)
                    + terms.d3210 * std::cos(xomi + xli - kG32) + terms.d3222 * std::cos(-xomi + xli - kG32)
                    + terms.d5220 * std::cos(xomi + xli - kG52) + terms.d5232 * std::cos(-xomi + xli - kG52)
                    + 2.0 * (terms.d4410 * std::cos(x2omi + x2li - kG44) + terms.d4422 * std::cos(x2li - kG44) + terms.d5421 * std::cos(xomi + x2li - kG54) + terms.d5433 * std::cos(-xomi + x2li - kG54));
                xnddt = xnddt * xldot;
            }

            if (std::fabs(t - atime) < kResonanceStepMinutes)
            {
                ft = t - atime;
                reached = true;
                break;
            }

            xli = xli + xldot * delt + xndt * kResonanceHalfStepSquared;
            xni = xni + xndt * delt + xnddt * kResonanceHalfStepSquared;
            atime = atime + delt;
        }

        if (!reached)
        {
            return false;
        }

        nm = xni + xndt * ft + xnddt * ft * ft * 0.5;
        const double xl = xli + xldot * ft + xndt * ft * ft * 0.5;

        if (terms.resonance == SDP4Resonance::Synchronous)
        {
            mm = xl - nodem - argpm + theta;
        }
        else
        {
            mm = xl - 2.0 * nodem + 2.0 * theta;
        }

        // Written the reference's way rather than folded, because nm - no and no + that are not
        // the same thing back again in floating point and the difference is measurable.
        const double dndt = nm - elementSet.no_unkozai;
        nm = elementSet.no_unkozai + dndt;
        return true;
    }

} // namespace

SGP4ElementSet SGP4Initialise(const SGP4Elements& elements)
{
    SGP4ElementSet elementSet;

    // The step reads the mean elements themselves at every call, so they travel with the
    // coefficients rather than being looked up separately.
    elementSet.bstar = elements.bstar;
    elementSet.ecco = elements.ecco;
    elementSet.inclo = elements.inclo;
    elementSet.nodeo = elements.nodeo;
    elementSet.argpo = elements.argpo;
    elementSet.mo = elements.mo;

    // The atmospheric model's boundary: the drag term is fitted between 78 km and 120 km altitude,
    // expressed in earth radii.
    const double ss = 78.0 / kSGP4EarthRadius + 1.0;
    const double qzms2ttemp = (120.0 - 78.0) / kSGP4EarthRadius;
    const double qzms2t = qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;

    // --- The preamble both branches share (the reference's initl) ---

    const double eccsq = elements.ecco * elements.ecco;
    const double omeosq = 1.0 - eccsq;
    const double rteosq = std::sqrt(omeosq);
    const double cosio = std::cos(elements.inclo);
    const double cosio2 = cosio * cosio;

    // Un-Kozai the mean motion. A TLE states the mean motion of Kozai's theory, which carries a
    // secular J2 term SGP4 accounts for separately; using it unaltered double-counts that term.
    // Two passes of the same correction, the second refining the semi-major axis the first found.
    const double ak = std::pow(kSGP4Xke / elements.noKozai, kTwoThirds);
    const double d1 = 0.75 * kSGP4J2 * (3.0 * cosio2 - 1.0) / (rteosq * omeosq);
    double del = d1 / (ak * ak);
    const double adel = ak * (1.0 - del * del - del * (1.0 / 3.0 + 134.0 * del * del / 81.0));
    del = d1 / (adel * adel);
    const double no_unkozai = elements.noKozai / (1.0 + del);

    const double ao = std::pow(kSGP4Xke / no_unkozai, kTwoThirds);
    const double sinio = std::sin(elements.inclo);
    const double po = ao * omeosq;
    const double con42 = 1.0 - 5.0 * cosio2;
    const double con41 = -con42 - cosio2 - cosio2;
    const double posq = po * po;
    const double rp = ao * (1.0 - elements.ecco);

    // Which algorithm this element set belongs to. Deep space is not a longer version of the near
    // earth path but an addition to it: everything below is computed for these orbits too, and
    // SDP4 then adds the lunar-solar and resonance terms on top. So the decision is recorded here
    // and acted on further down, where the reference acts on it.
    const bool deepSpace = glm::two_pi<double>() / no_unkozai >= kDeepSpacePeriodMinutes;

    elementSet.no_unkozai = no_unkozai;
    elementSet.con41 = con41;

    // The reference's own guard. It is an or, not an and, so it holds for anything short of both
    // an eccentricity past one and a negative mean motion; failing it leaves the coefficients zero.
    if ((omeosq >= 0.0) || (no_unkozai >= 0.0))
    {
        // Deep space takes the simplified drag model whatever its perigee. The higher order terms
        // model a decaying orbit over hours; nothing up here decays on that timescale, and the
        // reference forces the same flag for the same reason.
        elementSet.simplifiedDrag = deepSpace || rp < (kSimplifiedDragPerigee / kSGP4EarthRadius + 1.0);

        double sfour = ss;
        double qzms24 = qzms2t;
        const double perige = (rp - 1.0) * kSGP4EarthRadius;

        // Below 156 km the fitted atmospheric boundary is moved down to meet the orbit, and below
        // 98 km it stops moving and sits at 20 km.
        if (perige < 156.0)
        {
            sfour = perige - 78.0;
            if (perige < 98.0)
            {
                sfour = 20.0;
            }
            const double qzms24temp = (120.0 - sfour) / kSGP4EarthRadius;
            qzms24 = qzms24temp * qzms24temp * qzms24temp * qzms24temp;
            sfour = sfour / kSGP4EarthRadius + 1.0;
        }

        const double pinvsq = 1.0 / posq;
        const double tsi = 1.0 / (ao - sfour);
        elementSet.eta = ao * elements.ecco * tsi;
        const double etasq = elementSet.eta * elementSet.eta;
        const double eeta = elements.ecco * elementSet.eta;
        const double psisq = std::fabs(1.0 - etasq);
        const double coef = qzms24 * std::pow(tsi, 4.0);
        const double coef1 = coef / std::pow(psisq, 3.5);
        const double cc2 = coef1 * no_unkozai * (ao * (1.0 + 1.5 * etasq + eeta * (4.0 + etasq)) + 0.375 * kSGP4J2 * tsi / psisq * con41 * (8.0 + 3.0 * etasq * (8.0 + etasq)));
        elementSet.cc1 = elements.bstar * cc2;

        // cc3 and xmcof both divide by the eccentricity, so a near-circular orbit takes the terms
        // they feed as zero rather than as an overflow.
        double cc3 = 0.0;
        if (elements.ecco > 1.0e-4)
        {
            cc3 = -2.0 * coef * tsi * kSGP4J3OverJ2 * no_unkozai * sinio / elements.ecco;
        }

        elementSet.x1mth2 = 1.0 - cosio2;
        elementSet.cc4 = 2.0 * no_unkozai * coef1 * ao * omeosq * (elementSet.eta * (2.0 + 0.5 * etasq) + elements.ecco * (0.5 + 2.0 * etasq) - kSGP4J2 * tsi / (ao * psisq) * (-3.0 * con41 * (1.0 - 2.0 * eeta + etasq * (1.5 - 0.5 * eeta)) + 0.75 * elementSet.x1mth2 * (2.0 * etasq - eeta * (1.0 + etasq)) * std::cos(2.0 * elements.argpo)));
        elementSet.cc5 = 2.0 * coef1 * ao * omeosq * (1.0 + 2.75 * (etasq + eeta) + eeta * etasq);

        // The secular rates of the mean anomaly, argument of perigee and node under J2, J2 squared
        // and J4.
        const double cosio4 = cosio2 * cosio2;
        const double temp1 = 1.5 * kSGP4J2 * pinvsq * no_unkozai;
        const double temp2 = 0.5 * temp1 * kSGP4J2 * pinvsq;
        const double temp3 = -0.46875 * kSGP4J4 * pinvsq * pinvsq * no_unkozai;
        elementSet.mdot = no_unkozai + 0.5 * temp1 * rteosq * con41 + 0.0625 * temp2 * rteosq * (13.0 - 78.0 * cosio2 + 137.0 * cosio4);
        elementSet.argpdot = -0.5 * temp1 * con42 + 0.0625 * temp2 * (7.0 - 114.0 * cosio2 + 395.0 * cosio4) + temp3 * (3.0 - 36.0 * cosio2 + 49.0 * cosio4);
        const double xhdot1 = -temp1 * cosio;
        elementSet.nodedot = xhdot1 + (0.5 * temp2 * (4.0 - 19.0 * cosio2) + 2.0 * temp3 * (3.0 - 7.0 * cosio2)) * cosio;

        elementSet.omgcof = elements.bstar * cc3 * std::cos(elements.argpo);
        elementSet.xmcof = 0.0;
        if (elements.ecco > 1.0e-4)
        {
            elementSet.xmcof = -kTwoThirds * coef * elements.bstar / eeta;
        }
        elementSet.nodecf = 3.5 * omeosq * xhdot1 * elementSet.cc1;
        elementSet.t2cof = 1.5 * elementSet.cc1;

        // An exactly retrograde orbit sends (1 + cos i) to zero. Substituting the threshold itself
        // as the divisor keeps the result finite; it is arbitrary, and it is the reference's.
        if (std::fabs(cosio + 1.0) > kRetrogradeSingularityGuard)
        {
            elementSet.xlcof = -0.25 * kSGP4J3OverJ2 * sinio * (3.0 + 5.0 * cosio) / (1.0 + cosio);
        }
        else
        {
            elementSet.xlcof = -0.25 * kSGP4J3OverJ2 * sinio * (3.0 + 5.0 * cosio) / kRetrogradeSingularityGuard;
        }

        elementSet.aycof = -0.5 * kSGP4J3OverJ2 * sinio;
        const double delmotemp = 1.0 + elementSet.eta * std::cos(elements.mo);
        elementSet.delmo = delmotemp * delmotemp * delmotemp;
        elementSet.sinmao = std::sin(elements.mo);
        elementSet.x7thm1 = 7.0 * cosio2 - 1.0;

        // --- The other algorithm, for the orbits that need it ---
        //
        // Everything above is still read by the deep-space step; what follows is added to it. The
        // ordering is the reference's: this sits between the last near-earth coefficient and the
        // higher order drag terms, which the flag set above has already excluded it from.
        if (deepSpace)
        {
            elementSet.method = SGP4Method::DeepSpace;

            // The combined precession rate of the orbit's own plane and line of apsides, which is
            // what a synchronous resonance beats against.
            const double xpidot = elementSet.argpdot + elementSet.nodedot;

            elementSet.deepSpace.gsto = SiderealTimeAtEpoch(elements.epochDaysSince1950);

            const DeepSpaceCommonTerms common = DeepSpaceCommon(elements, no_unkozai, elementSet.deepSpace);
            DeepSpaceInitialise(elements, no_unkozai, elementSet.mdot, elementSet.nodedot, xpidot, common, elementSet.deepSpace);
        }

        // The higher order drag terms, which a fast-decaying orbit does without.
        if (!elementSet.simplifiedDrag)
        {
            const double cc1sq = elementSet.cc1 * elementSet.cc1;
            elementSet.d2 = 4.0 * ao * tsi * cc1sq;
            const double temp = elementSet.d2 * tsi * elementSet.cc1 / 3.0;
            elementSet.d3 = (17.0 * ao + sfour) * temp;
            elementSet.d4 = 0.5 * temp * ao * tsi * (221.0 * ao + 31.0 * sfour) * elementSet.cc1;
            elementSet.t3cof = elementSet.d2 + 2.0 * cc1sq;
            elementSet.t4cof = 0.25 * (3.0 * elementSet.d3 + elementSet.cc1 * (12.0 * elementSet.d2 + 10.0 * cc1sq));
            elementSet.t5cof = 0.2 * (3.0 * elementSet.d4 + 12.0 * elementSet.cc1 * elementSet.d3 + 6.0 * elementSet.d2 * elementSet.d2 + 15.0 * cc1sq * (2.0 * elementSet.d2 + cc1sq));
        }
    }

    return elementSet;
}

SGP4Position SGP4Step(const SGP4ElementSet& elementSet, double tsinceMinutes)
{
    SGP4Position result;

    const double twopi = glm::two_pi<double>();

    // Earth radii per minute into km per second, which is the only place the velocity's units
    // come from.
    const double vkmpersec = kSGP4EarthRadius * kSGP4Xke / 60.0;

    const double t = tsinceMinutes;

    // --- Secular gravity and atmospheric drag ---

    const double xmdf = elementSet.mo + elementSet.mdot * t;
    const double argpdf = elementSet.argpo + elementSet.argpdot * t;
    const double nodedf = elementSet.nodeo + elementSet.nodedot * t;
    double argpm = argpdf;
    double mm = xmdf;
    const double t2 = t * t;
    double nodem = nodedf + elementSet.nodecf * t2;
    double tempa = 1.0 - elementSet.cc1 * t;
    double tempe = elementSet.bstar * elementSet.cc4 * t;
    double templ = elementSet.t2cof * t2;

    // The higher order drag terms, which the coefficients only exist for when the orbit was not
    // decaying fast enough to have them dropped at initialisation.
    if (!elementSet.simplifiedDrag)
    {
        const double delomg = elementSet.omgcof * t;
        const double delmtemp = 1.0 + elementSet.eta * std::cos(xmdf);
        const double delm = elementSet.xmcof * (delmtemp * delmtemp * delmtemp - elementSet.delmo);
        const double temp = delomg + delm;
        mm = xmdf + temp;
        argpm = argpdf - temp;
        const double t3 = t2 * t;
        const double t4 = t3 * t;
        tempa = tempa - elementSet.d2 * t2 - elementSet.d3 * t3 - elementSet.d4 * t4;
        tempe = tempe + elementSet.bstar * elementSet.cc5 * (std::sin(mm) - elementSet.sinmao);
        templ = templ + elementSet.t3cof * t3 + t4 * (elementSet.t4cof + t * elementSet.t5cof);
    }

    double nm = elementSet.no_unkozai;
    double em = elementSet.ecco;
    double inclm = elementSet.inclo;

    // The Sun and the Moon, and the Earth's own shape for the few orbits in resonance with it. All
    // of these move the mean elements secularly, so they belong here with drag rather than with
    // the periodics further down. Before the mean motion check because a resonance changes the
    // mean motion, which is the thing being checked.
    if (elementSet.method == SGP4Method::DeepSpace)
    {
        if (!DeepSpaceSecular(elementSet, t, em, inclm, argpm, nodem, mm, nm))
        {
            result.error = SGP4Error::ResonanceStepLimitExceeded;
            return result;
        }
    }

    if (nm <= 0.0)
    {
        result.error = SGP4Error::MeanMotionNotPositive;
        return result;
    }

    // The drag correction has changed sign, which is meaningless - and dangerous rather than
    // merely wrong, because am squares it and so turns it into a large semi-major axis rather
    // than a small one. See SGP4Error::DragModelDiverged for why the reference does not make this
    // check and why the threshold is zero. Placed after the mean motion check rather than before
    // it so that an element set failing both still reports what the reference reports.
    if (tempa < 0.0)
    {
        result.error = SGP4Error::DragModelDiverged;
        return result;
    }

    const double am = std::pow((kSGP4Xke / nm), kTwoThirds) * tempa * tempa;
    nm = kSGP4Xke / std::pow(am, 1.5);
    em = em - tempe;

    // Drag has taken the eccentricity somewhere an orbit cannot be, or the semi-major axis below
    // the ground. The small negative bound is the reference's tolerance rather than a physical
    // statement. The 0.95 earth radii is Vallado's own - documented as part of his error 1, though
    // commented out in his code for reasons noted at SGP4Error::DragModelDiverged - and is enabled
    // here because, unlike the mrt < 1 test at the end of the step, it does not depend on where in
    // its orbit the object happens to be: an eccentric orbit whose semi-major axis has collapsed
    // cannot slip past it by being near apogee. It costs nothing against the verification file,
    // where the lowest am any near-earth case reaches is 0.9956.
    if ((em >= 1.0) || (em < -0.001) || (am < 0.95))
    {
        result.error = SGP4Error::MeanElementsOutOfRange;
        return result;
    }

    if (em < 1.0e-6)
    {
        em = 1.0e-6;
    }

    mm = mm + elementSet.no_unkozai * templ;
    double xlm = mm + argpm + nodem;

    nodem = std::fmod(nodem, twopi);
    argpm = std::fmod(argpm, twopi);
    xlm = std::fmod(xlm, twopi);
    mm = std::fmod(xlm - argpm - nodem, twopi);

    const double sinim = std::sin(inclm);
    const double cosim = std::cos(inclm);

    // The lunar-solar periodics are added to copies rather than to the mean elements themselves,
    // which is what these are for: what follows is the osculating orbit at this instant, while the
    // mean elements stay the smooth thing the secular rates above were applied to.
    double ep = em;
    double xincp = inclm;
    double argpp = argpm;
    double nodep = nodem;
    double mp = mm;
    double sinip = sinim;
    double cosip = cosim;

    // Five coefficients initialisation produced that the deep-space branch has to produce again,
    // because all five are functions of the inclination and the periodics have just moved it. They
    // are locals rather than writes back into the element set - which is what the reference does -
    // so that the step stays something that can be run on anything, in any order, for any time.
    double aycof = elementSet.aycof;
    double xlcof = elementSet.xlcof;
    double con41 = elementSet.con41;
    double x1mth2 = elementSet.x1mth2;
    double x7thm1 = elementSet.x7thm1;

    if (elementSet.method == SGP4Method::DeepSpace)
    {
        DeepSpacePeriodics(elementSet.deepSpace, t, ep, xincp, nodep, argpp, mp);

        // Lyddane's formulation can take the inclination through zero and out the other side,
        // which is the same orbit described backwards. Reflecting it back costs a half turn in
        // both the node and the argument of perigee.
        if (xincp < 0.0)
        {
            xincp = -xincp;
            nodep = nodep + glm::pi<double>();
            argpp = argpp - glm::pi<double>();
        }

        if ((ep < 0.0) || (ep > 1.0))
        {
            result.error = SGP4Error::PerturbedEccentricityOutOfRange;
            return result;
        }

        sinip = std::sin(xincp);
        cosip = std::cos(xincp);
        aycof = -0.5 * kSGP4J3OverJ2 * sinip;

        if (std::fabs(cosip + 1.0) > kRetrogradeSingularityGuard)
        {
            xlcof = -0.25 * kSGP4J3OverJ2 * sinip * (3.0 + 5.0 * cosip) / (1.0 + cosip);
        }
        else
        {
            xlcof = -0.25 * kSGP4J3OverJ2 * sinip * (3.0 + 5.0 * cosip) / kRetrogradeSingularityGuard;
        }
    }

    // --- Long period periodics ---

    const double axnl = ep * std::cos(argpp);
    double temp = 1.0 / (am * (1.0 - ep * ep));
    const double aynl = ep * std::sin(argpp) + temp * aycof;
    const double xl = mp + argpp + nodep + temp * xlcof * axnl;

    // --- Kepler's equation ---
    //
    // Newton-Raphson, capped at ten passes and with each correction clamped, because this is
    // solved for thirty thousand objects a frame and an orbit that converges slowly must cost a
    // bounded amount rather than an unbounded one.
    const double u = std::fmod(xl - nodep, twopi);
    double eo1 = u;
    double tem5 = 9999.9;
    double sineo1 = 0.0;
    double coseo1 = 0.0;
    int ktr = 1;
    while ((std::fabs(tem5) >= 1.0e-12) && (ktr <= 10))
    {
        sineo1 = std::sin(eo1);
        coseo1 = std::cos(eo1);
        tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
        tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;
        if (std::fabs(tem5) >= 0.95)
        {
            tem5 = tem5 > 0.0 ? 0.95 : -0.95;
        }
        eo1 = eo1 + tem5;
        ktr = ktr + 1;
    }

    // --- Short period periodics ---

    const double ecose = axnl * coseo1 + aynl * sineo1;
    const double esine = axnl * sineo1 - aynl * coseo1;
    const double el2 = axnl * axnl + aynl * aynl;
    const double pl = am * (1.0 - el2);

    if (pl < 0.0)
    {
        result.error = SGP4Error::NegativeSemiLatusRectum;
        return result;
    }

    const double rl = am * (1.0 - ecose);
    const double rdotl = std::sqrt(am) * esine / rl;
    const double rvdotl = std::sqrt(pl) / rl;
    const double betal = std::sqrt(1.0 - el2);
    temp = esine / (1.0 + betal);
    const double sinu = am / rl * (sineo1 - aynl - axnl * temp);
    const double cosu = am / rl * (coseo1 - axnl + aynl * temp);
    double su = std::atan2(sinu, cosu);
    const double sin2u = (cosu + cosu) * sinu;
    const double cos2u = 1.0 - 2.0 * sinu * sinu;
    temp = 1.0 / pl;
    const double temp1 = 0.5 * kSGP4J2 * temp;
    const double temp2 = temp1 * temp;

    // The other three, recomputed here rather than above because this is where the reference does
    // it and the operation order is what the exact comparison rests on.
    if (elementSet.method == SGP4Method::DeepSpace)
    {
        const double cosisq = cosip * cosip;
        con41 = 3.0 * cosisq - 1.0;
        x1mth2 = 1.0 - cosisq;
        x7thm1 = 7.0 * cosisq - 1.0;
    }

    const double mrt = rl * (1.0 - 1.5 * temp2 * betal * con41) + 0.5 * temp1 * x1mth2 * cos2u;
    su = su - 0.25 * temp2 * x7thm1 * sin2u;
    const double xnode = nodep + 1.5 * temp2 * cosip * sin2u;
    const double xinc = xincp + 1.5 * temp2 * cosip * sinip * cos2u;
    const double mvt = rdotl - nm * temp1 * x1mth2 * sin2u / kSGP4Xke;
    const double rvdot = rvdotl + nm * temp1 * (x1mth2 * cos2u + 1.5 * con41) / kSGP4Xke;

    // --- Orientation vectors ---

    const double sinsu = std::sin(su);
    const double cossu = std::cos(su);
    const double snod = std::sin(xnode);
    const double cnod = std::cos(xnode);
    const double sini = std::sin(xinc);
    const double cosi = std::cos(xinc);
    const double xmx = -snod * cosi;
    const double xmy = cnod * cosi;
    const double ux = xmx * sinsu + cnod * cossu;
    const double uy = xmy * sinsu + snod * cossu;
    const double uz = sini * sinsu;
    const double vx = xmx * cossu - cnod * sinsu;
    const double vy = xmy * cossu - snod * sinsu;
    const double vz = sini * cossu;

    result.position = glm::dvec3(mrt * ux, mrt * uy, mrt * uz) * kSGP4EarthRadius;
    result.velocity = glm::dvec3(mvt * ux + rvdot * vx, mvt * uy + rvdot * vy, mvt * uz + rvdot * vz) * vkmpersec;

    // Below one earth radius, so the orbit has come down. Reported after the position rather than
    // instead of it, because the position is how it was noticed.
    if (mrt < 1.0)
    {
        result.error = SGP4Error::Decayed;
    }

    return result;
}

SGP4Elements MakeSGP4Elements(const OrbitalElementsComponent& orbitalElements)
{
    // 1950 January 0.0 - JD 2433281.5, and the day count SGP4 initialises from - is 7306 days
    // before the Unix epoch. Not the 7305 the reference subtracts on its way to the sidereal time
    // at epoch: that one counts back to 1970 January 0.0, which is the last day of 1969 rather
    // than the first of 1970. The two dates are a day apart and so are the two constants.
    constexpr double kDaysFrom1950ToUnixEpoch = 7306.0;
    constexpr double kSecondsPerDay = 86400.0;

    const double degreesToRadians = glm::pi<double>() / 180.0;

    SGP4Elements elements;
    elements.bstar = orbitalElements.GetBStar();
    elements.ecco = orbitalElements.GetEccentricity();
    elements.argpo = orbitalElements.GetArgumentOfPericenter() * degreesToRadians;
    elements.inclo = orbitalElements.GetInclination() * degreesToRadians;
    elements.mo = orbitalElements.GetMeanAnomaly() * degreesToRadians;
    elements.nodeo = orbitalElements.GetRightAscensionOfAscendingNode() * degreesToRadians;
    elements.noKozai = orbitalElements.GetMeanMotion() / kRevsPerDayToRadsPerMinute;

    const double secondsSinceUnixEpoch = std::chrono::duration<double>(orbitalElements.GetEpoch().time_since_epoch()).count();
    elements.epochDaysSince1950 = secondsSinceUnixEpoch / kSecondsPerDay + kDaysFrom1950ToUnixEpoch;

    return elements;
}

} // namespace WingsOfSteel
