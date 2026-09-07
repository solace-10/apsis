// The SGP4 step, near-earth branch.
//
// The other half - turning an element set into these coefficients - runs on the CPU in double,
// once per object, and is in game/src/space/sgp4.cpp. That split is the whole design: WGSL has no
// f64, so the less of a double-precision algorithm that has to survive f32 the better, and
// initialisation is both the more delicate half and the half that does not need doing per frame.
//
// This is a transcription of SGP4Step() in that file, which is itself checked against Vallado's
// reference bit for bit. So anything this disagrees with it about is f32 and nothing else, which
// is what game/tests/space/sgp4_shader_tests.cpp measures.

// Mirrors SGP4StepInput in game/src/render/sgp4_compute_pass.hpp.
// The two must be kept in step; a static_assert there pins the size.
struct SGP4ElementSet
{
    simplifiedDrag: u32,
    deepSpace: u32,
    bstar: f32,
    ecco: f32,
    inclo: f32,
    nodeo: f32,
    argpo: f32,
    mo: f32,
    no_unkozai: f32,
    aycof: f32,
    con41: f32,
    cc1: f32,
    cc4: f32,
    cc5: f32,
    d2: f32,
    d3: f32,
    d4: f32,
    delmo: f32,
    eta: f32,
    argpdot: f32,
    omgcof: f32,
    sinmao: f32,
    t2cof: f32,
    t3cof: f32,
    t4cof: f32,
    t5cof: f32,
    x1mth2: f32,
    x7thm1: f32,
    mdot: f32,
    nodedot: f32,
    xlcof: f32,
    xmcof: f32,
    nodecf: f32,

    // SDP4Terms, flattened - see SGP4StepInput. Zero throughout for a near-earth element set, and
    // uploaded either way so that one struct serves both branches and the roster stays one roster.
    resonance: u32,
    dedt: f32,
    didt: f32,
    dmdt: f32,
    dnodt: f32,
    domdt: f32,
    e3: f32,
    ee2: f32,
    se2: f32,
    se3: f32,
    sgh2: f32,
    sgh3: f32,
    sgh4: f32,
    sh2: f32,
    sh3: f32,
    si2: f32,
    si3: f32,
    sl2: f32,
    sl3: f32,
    sl4: f32,
    xgh2: f32,
    xgh3: f32,
    xgh4: f32,
    xh2: f32,
    xh3: f32,
    xi2: f32,
    xi3: f32,
    xl2: f32,
    xl3: f32,
    xl4: f32,
    zmol: f32,
    zmos: f32,
    d2201: f32,
    d2211: f32,
    d3210: f32,
    d3222: f32,
    d4410: f32,
    d4422: f32,
    d5220: f32,
    d5232: f32,
    d5421: f32,
    d5433: f32,
    del1: f32,
    del2: f32,
    del3: f32,
    gsto: f32,
    xfact: f32,
    xlamo: f32,

    padding0: f32,
    padding1: f32,
    padding2: f32,
}

// Mirrors SGP4StepOutput. A vec3f aligns to 16 bytes, so both trailing slots are space the struct
// occupies regardless; the first is spent on the error, which there is no other way to report.
struct PropagatedState
{
    position: vec3f,
    error: u32,
    velocity: vec3f,
    padding: f32,
}

// Mirrors SGP4Error in game/src/space/sgp4.hpp, which states its values for this reason.
const kErrorNone: u32 = 0u;
const kErrorDeepSpaceNotSupported: u32 = 1u;
const kErrorMeanMotionNotPositive: u32 = 2u;
const kErrorMeanElementsOutOfRange: u32 = 3u;
const kErrorNegativeSemiLatusRectum: u32 = 4u;
const kErrorDecayed: u32 = 5u;
const kErrorDragModelDiverged: u32 = 6u;
const kErrorPerturbedEccentricityOutOfRange: u32 = 7u;
const kErrorResonanceStepLimitExceeded: u32 = 8u;

// WGS72, the model TLEs are fitted with. Written out to the precision of the double the CPU side
// uses, so that the compiler rounds the same value rather than rounding a different one.
const kEarthRadius: f32 = 6378.135;
const kXke: f32 = 0.074366916133173422;
const kJ2: f32 = 0.001082616;

const kTwoPi: f32 = 6.283185307179586;
const kPi: f32 = 3.141592653589793;
const kTwoThirds: f32 = 0.6666666666666666;

// --- Deep space ---
//
// All of these mirror sgp4.cpp, which carries the reasoning for each; only the ones the STEP needs
// are here, so the dsinit-only amplitudes (kQ22, the kRoot family) are absent.
const kJ3OverJ2: f32 = -0.002345069720011528;
const kRetrogradeSingularityGuard: f32 = 1.5e-12;

// The Sun's and Moon's mean motions and eccentricities, for the periodics.
const kSolarMeanMotion: f32 = 1.19459e-5;
const kSolarEccentricity: f32 = 0.01675;
const kLunarMeanMotion: f32 = 1.5835218e-4;
const kLunarEccentricity: f32 = 0.05490;

// Above this inclination the node and argument of perigee are corrected directly; below it,
// Lyddane's formulation is used instead. 0.2 rad, the reference's threshold.
const kLyddaneInclinationThreshold: f32 = 0.2;

// The Earth's rotation rate in radians per minute - what a resonance is in resonance with - and
// the phase angles of the tesseral harmonics the two arms run on.
const kEarthRotationRate: f32 = 4.37526908801129966e-3;
const kFasx2: f32 = 0.13130908;
const kFasx4: f32 = 2.8843198;
const kFasx6: f32 = 0.37448087;
const kG22: f32 = 5.7686396;
const kG32: f32 = 0.95240898;
const kG44: f32 = 1.8014998;
const kG52: f32 = 1.0508330;
const kG54: f32 = 4.4108898;

// SDP4Resonance's values, which sgp4.hpp states for this reason.
const kResonanceNone: u32 = 0u;
const kResonanceSynchronous: u32 = 1u;
const kResonanceHalfDay: u32 = 2u;

// The resonance integrator's step and its second order coefficient, and the cap on how many steps
// it may take. SGP4Step() keeps the same cap deliberately - a bound here that the CPU did not have
// would leave the two no longer comparable, which is what sgp4_shader_tests.cpp rests on. The
// arithmetic behind 72 is in sgp4.cpp beside kResonanceMaxSteps; in short it is how far from its
// epoch an element set can be before the ingestion should already have dropped it.
const kResonanceStepMinutes: f32 = 720.0;
const kResonanceHalfStepSquared: f32 = 259200.0;
const kResonanceMaxSteps: i32 = 72;

// Where the Kepler solve below stops. Deliberately not the CPU's 1e-12: eo1 runs to 2*pi, so its
// f32 ULP is around 5e-7 and a correction below that cannot move it. A tolerance under the ULP is
// one the loop stalls above rather than reaches, which is what kept it running all ten passes.
const kKeplerTolerance: f32 = 1e-6;

@group(0) @binding(0) var<storage, read> elementSets: array<SGP4ElementSet>;

// Minutes from each element set's own epoch. Its own buffer because it is the only thing here that
// changes every frame; the coefficients are uploaded once and left alone.
@group(0) @binding(1) var<storage, read> times: array<f32>;

@group(0) @binding(2) var<storage, read_write> propagatedStates: array<PropagatedState>;

fn failed(error: u32) -> PropagatedState {
    var state: PropagatedState;
    state.position = vec3f(0.0);
    state.velocity = vec3f(0.0);
    state.error = error;
    state.padding = 0.0;
    return state;
}


// The lunar-solar periodics (dpper), transcribed from DeepSpacePeriodics() in sgp4.cpp.
//
// WGSL has no output parameters, so what the CPU passes by reference comes back in a struct. The
// arithmetic and its order are otherwise the same, which is what lets sgp4_shader_tests.cpp read
// the difference between the two as f32 and nothing else.
struct DeepSpacePeriodics
{
    ep: f32,
    inclp: f32,
    nodep: f32,
    argpp: f32,
    mp: f32,
}

fn deepSpacePeriodics(e: SGP4ElementSet, t: f32, epIn: f32, inclpIn: f32, nodepIn: f32, argppIn: f32, mpIn: f32) -> DeepSpacePeriodics {
    var ep = epIn;
    var inclp = inclpIn;
    var nodep = nodepIn;
    var argpp = argppIn;
    var mp = mpIn;

    // The Sun, advanced from the epoch to now, and its equation of the centre to first order.
    var zm = e.zmos + kSolarMeanMotion * t;
    var zf = zm + 2.0 * kSolarEccentricity * sin(zm);
    var sinzf = sin(zf);
    var f2 = 0.5 * sinzf * sinzf - 0.25;
    var f3 = -0.5 * sinzf * cos(zf);
    let ses = e.se2 * f2 + e.se3 * f3;
    let sis = e.si2 * f2 + e.si3 * f3;
    let sls = e.sl2 * f2 + e.sl3 * f3 + e.sl4 * sinzf;
    let sghs = e.sgh2 * f2 + e.sgh3 * f3 + e.sgh4 * sinzf;
    let shs = e.sh2 * f2 + e.sh3 * f3;

    // And the Moon.
    zm = e.zmol + kLunarMeanMotion * t;
    zf = zm + 2.0 * kLunarEccentricity * sin(zm);
    sinzf = sin(zf);
    f2 = 0.5 * sinzf * sinzf - 0.25;
    f3 = -0.5 * sinzf * cos(zf);
    let sel = e.ee2 * f2 + e.e3 * f3;
    let sil = e.xi2 * f2 + e.xi3 * f3;
    let sll = e.xl2 * f2 + e.xl3 * f3 + e.xl4 * sinzf;
    let sghl = e.xgh2 * f2 + e.xgh3 * f3 + e.xgh4 * sinzf;
    let shll = e.xh2 * f2 + e.xh3 * f3;

    let pe = ses + sel;
    let pinc = sis + sil;
    let pl = sls + sll;
    var pgh = sghs + sghl;
    var ph = shs + shll;

    inclp = inclp + pinc;
    ep = ep + pe;
    let sinip = sin(inclp);
    let cosip = cos(inclp);

    // Two ways of applying the same corrections. Above about eleven and a half degrees the node and
    // the argument of perigee are separated well enough to be corrected directly; below it they are
    // not, and Lyddane's formulation - which corrects the combination that stays well defined as
    // the inclination goes to zero - is used instead.
    if (inclp >= kLyddaneInclinationThreshold) {
        ph = ph / sinip;
        pgh = pgh - cosip * ph;
        argpp = argpp + pgh;
        nodep = nodep + ph;
        mp = mp + pl;
    } else {
        let sinop = sin(nodep);
        let cosop = cos(nodep);
        var alfdp = sinip * sinop;
        var betdp = sinip * cosop;
        let dalf = ph * cosop + pinc * cosip * sinop;
        let dbet = -ph * sinop + pinc * cosip * cosop;
        alfdp = alfdp + dalf;
        betdp = betdp + dbet;
        nodep = nodep % kTwoPi;

        var xls = mp + argpp + cosip * nodep;
        let dls = pl + pgh - pinc * nodep * sinip;
        xls = xls + dls;
        let xnoh = nodep;
        nodep = atan2(alfdp, betdp);

        // Keeping the node on the same turn it was on before atan2 put it back in range.
        if (abs(xnoh - nodep) > kPi) {
            if (nodep < xnoh) {
                nodep = nodep + kTwoPi;
            } else {
                nodep = nodep - kTwoPi;
            }
        }

        mp = mp + pl;
        argpp = xls - mp - cosip * nodep;
    }

    return DeepSpacePeriodics(ep, inclp, nodep, argpp, mp);
}

// The secular contribution of the Sun, the Moon and - for a resonant orbit - the Earth's own shape
// (dspace), transcribed from DeepSpaceSecular() in sgp4.cpp.
//
// reached is false when the integration could not get to t inside kResonanceMaxSteps, which the
// caller turns into kErrorResonanceStepLimitExceeded. The CPU returns the same thing as a bool.
struct DeepSpaceSecular
{
    em: f32,
    inclm: f32,
    argpm: f32,
    nodem: f32,
    mm: f32,
    nm: f32,
    reached: bool,
}

fn deepSpaceSecular(e: SGP4ElementSet, t: f32, emIn: f32, inclmIn: f32, argpmIn: f32, nodemIn: f32, mmIn: f32, nmIn: f32) -> DeepSpaceSecular {
    var em = emIn;
    var inclm = inclmIn;
    var argpm = argpmIn;
    var nodem = nodemIn;
    var mm = mmIn;
    var nm = nmIn;

    let theta = (e.gsto + t * kEarthRotationRate) % kTwoPi;

    em = em + e.dedt * t;
    inclm = inclm + e.didt * t;
    argpm = argpm + e.domdt * t;
    nodem = nodem + e.dnodt * t;
    mm = mm + e.dmdt * t;

    if (e.resonance == kResonanceNone) {
        return DeepSpaceSecular(em, inclm, argpm, nodem, mm, nm, true);
    }

    // Euler-Maclaurin, in whole steps of twelve hours towards the time asked for and then a partial
    // one to land on it. Restarted from the epoch on every call rather than resumed - SDP4Terms in
    // sgp4.hpp explains why that is both correct and affordable.
    var atime = 0.0;
    var xli = e.xlamo;
    var xni = e.no_unkozai;
    var delt = -kResonanceStepMinutes;
    if (t > 0.0) {
        delt = kResonanceStepMinutes;
    }

    var xndt = 0.0;
    var xldot = 0.0;
    var xnddt = 0.0;
    var ft = 0.0;
    var reached = false;

    for (var step = 0; step <= kResonanceMaxSteps; step++) {
        if (e.resonance == kResonanceSynchronous) {
            xndt = e.del1 * sin(xli - kFasx2) + e.del2 * sin(2.0 * (xli - kFasx4)) + e.del3 * sin(3.0 * (xli - kFasx6));
            xldot = xni + e.xfact;
            xnddt = e.del1 * cos(xli - kFasx2) + 2.0 * e.del2 * cos(2.0 * (xli - kFasx4)) + 3.0 * e.del3 * cos(3.0 * (xli - kFasx6));
            xnddt = xnddt * xldot;
        } else {
            let xomi = e.argpo + e.argpdot * atime;
            let x2omi = xomi + xomi;
            let x2li = xli + xli;
            xndt = e.d2201 * sin(x2omi + xli - kG22) + e.d2211 * sin(xli - kG22)
                + e.d3210 * sin(xomi + xli - kG32) + e.d3222 * sin(-xomi + xli - kG32)
                + e.d4410 * sin(x2omi + x2li - kG44) + e.d4422 * sin(x2li - kG44)
                + e.d5220 * sin(xomi + xli - kG52) + e.d5232 * sin(-xomi + xli - kG52)
                + e.d5421 * sin(xomi + x2li - kG54) + e.d5433 * sin(-xomi + x2li - kG54);
            xldot = xni + e.xfact;
            xnddt = e.d2201 * cos(x2omi + xli - kG22) + e.d2211 * cos(xli - kG22)
                + e.d3210 * cos(xomi + xli - kG32) + e.d3222 * cos(-xomi + xli - kG32)
                + e.d5220 * cos(xomi + xli - kG52) + e.d5232 * cos(-xomi + xli - kG52)
                + 2.0 * (e.d4410 * cos(x2omi + x2li - kG44) + e.d4422 * cos(x2li - kG44) + e.d5421 * cos(xomi + x2li - kG54) + e.d5433 * cos(-xomi + x2li - kG54));
            xnddt = xnddt * xldot;
        }

        if (abs(t - atime) < kResonanceStepMinutes) {
            ft = t - atime;
            reached = true;
            break;
        }

        xli = xli + xldot * delt + xndt * kResonanceHalfStepSquared;
        xni = xni + xndt * delt + xnddt * kResonanceHalfStepSquared;
        atime = atime + delt;
    }

    if (!reached) {
        return DeepSpaceSecular(em, inclm, argpm, nodem, mm, nm, false);
    }

    nm = xni + xndt * ft + xnddt * ft * ft * 0.5;
    let xl = xli + xldot * ft + xndt * ft * ft * 0.5;

    if (e.resonance == kResonanceSynchronous) {
        mm = xl - nodem - argpm + theta;
    } else {
        mm = xl - 2.0 * nodem + 2.0 * theta;
    }

    // Written the reference's way rather than folded, because nm - no and no + that are not the
    // same thing back again in floating point and the difference is measurable.
    let dndt = nm - e.no_unkozai;
    nm = e.no_unkozai + dndt;

    return DeepSpaceSecular(em, inclm, argpm, nodem, mm, nm, true);
}

fn propagate(e: SGP4ElementSet, t: f32) -> PropagatedState {
    let vkmpersec = kEarthRadius * kXke / 60.0;

    // --- Secular gravity and atmospheric drag ---

    let xmdf = e.mo + e.mdot * t;
    let argpdf = e.argpo + e.argpdot * t;
    let nodedf = e.nodeo + e.nodedot * t;
    var argpm = argpdf;
    var mm = xmdf;
    let t2 = t * t;
    var nodem = nodedf + e.nodecf * t2;
    var tempa = 1.0 - e.cc1 * t;
    var tempe = e.bstar * e.cc4 * t;
    var templ = e.t2cof * t2;

    if (e.simplifiedDrag == 0u) {
        let delomg = e.omgcof * t;
        let delmtemp = 1.0 + e.eta * cos(xmdf);
        let delm = e.xmcof * (delmtemp * delmtemp * delmtemp - e.delmo);
        let temp = delomg + delm;
        mm = xmdf + temp;
        argpm = argpdf - temp;
        let t3 = t2 * t;
        let t4 = t3 * t;
        tempa = tempa - e.d2 * t2 - e.d3 * t3 - e.d4 * t4;
        tempe = tempe + e.bstar * e.cc5 * (sin(mm) - e.sinmao);
        templ = templ + e.t3cof * t3 + t4 * (e.t4cof + t * e.t5cof);
    }

    var nm = e.no_unkozai;
    var em = e.ecco;
    var inclm = e.inclo;

    // The Sun, the Moon, and the Earth's own shape for the few orbits in resonance with it. All of
    // these move the mean elements secularly, so they belong here with drag rather than with the
    // periodics further down. Before the mean motion check because a resonance changes the mean
    // motion, which is the thing being checked.
    if (e.deepSpace != 0u) {
        let secular = deepSpaceSecular(e, t, em, inclm, argpm, nodem, mm, nm);
        if (!secular.reached) {
            return failed(kErrorResonanceStepLimitExceeded);
        }
        em = secular.em;
        inclm = secular.inclm;
        argpm = secular.argpm;
        nodem = secular.nodem;
        mm = secular.mm;
        nm = secular.nm;
    }

    if (nm <= 0.0) {
        return failed(kErrorMeanMotionNotPositive);
    }

    // The drag correction has changed sign. This is the one check here the reference does not
    // make, and the only one that can see an object flung outwards by a truncated polynomial
    // rather than collapsing inwards - see SGP4Error::DragModelDiverged in sgp4.hpp.
    if (tempa < 0.0) {
        return failed(kErrorDragModelDiverged);
    }

    let am = pow((kXke / nm), kTwoThirds) * tempa * tempa;
    nm = kXke / pow(am, 1.5);
    em = em - tempe;

    if ((em >= 1.0) || (em < -0.001) || (am < 0.95)) {
        return failed(kErrorMeanElementsOutOfRange);
    }

    em = max(em, 1.0e-6);

    mm = mm + e.no_unkozai * templ;
    var xlm = mm + argpm + nodem;

    nodem = nodem % kTwoPi;
    argpm = argpm % kTwoPi;
    xlm = xlm % kTwoPi;
    mm = (xlm - argpm - nodem) % kTwoPi;

    var sinip = sin(inclm);
    var cosip = cos(inclm);

    var ep = em;
    var xincp = inclm;
    var argpp = argpm;
    var nodep = nodem;
    var mp = mm;

    // Two coefficients initialisation produced that the deep-space branch has to produce again,
    // because both are functions of the inclination and the periodics have just moved it. Locals
    // rather than writes back into the element set - which is what the reference does - so that the
    // step stays something that can be run on anything, in any order, for any time. con41, x1mth2
    // and x7thm1 are functions of the inclination too, but neither the reference nor SGP4Step()
    // recomputes them here, so they are still read straight off the struct below.
    var aycof = e.aycof;
    var xlcof = e.xlcof;

    if (e.deepSpace != 0u) {
        let periodics = deepSpacePeriodics(e, t, ep, xincp, nodep, argpp, mp);
        ep = periodics.ep;
        xincp = periodics.inclp;
        nodep = periodics.nodep;
        argpp = periodics.argpp;
        mp = periodics.mp;

        // Lyddane's formulation can take the inclination through zero and out the other side, which
        // is the same orbit described backwards. Reflecting it back costs a half turn in both the
        // node and the argument of perigee.
        if (xincp < 0.0) {
            xincp = -xincp;
            nodep = nodep + kPi;
            argpp = argpp - kPi;
        }

        if ((ep < 0.0) || (ep > 1.0)) {
            return failed(kErrorPerturbedEccentricityOutOfRange);
        }

        sinip = sin(xincp);
        cosip = cos(xincp);
        aycof = -0.5 * kJ3OverJ2 * sinip;

        if (abs(cosip + 1.0) > kRetrogradeSingularityGuard) {
            xlcof = -0.25 * kJ3OverJ2 * sinip * (3.0 + 5.0 * cosip) / (1.0 + cosip);
        } else {
            xlcof = -0.25 * kJ3OverJ2 * sinip * (3.0 + 5.0 * cosip) / kRetrogradeSingularityGuard;
        }
    }

    // --- Long period periodics ---

    let axnl = ep * cos(argpp);
    var temp = 1.0 / (am * (1.0 - ep * ep));
    let aynl = ep * sin(argpp) + temp * aycof;
    let xl = mp + argpp + nodep + temp * xlcof * axnl;

    // --- Kepler's equation ---
    //
    // Newton-Raphson, capped at ten passes and with each correction clamped, as in SGP4Step(). It
    // exits on kKeplerTolerance rather than the CPU's 1e-12, which f32 cannot reach: measured over
    // the near-earth cases in SGP4-VER.TLE, 1e-6 is crossed on the second pass for 126 of 160
    // samples and the third for the other 34, so the cap is there for an orbit that converges
    // slowly rather than for the ordinary case.
    //
    // The test breaks after the correction is applied because that is what the CPU loop does: its
    // while condition tests the correction the previous pass applied, so both leave sineo1/coseo1
    // one step behind eo1, and both stop on the same pass given the same tolerance.
    //
    // The saving is per wave rather than per object - lanes that converge early wait for the
    // slowest in their group - so this is worth less than the iteration counts suggest.
    let u = (xl - nodep) % kTwoPi;
    var eo1 = u;
    var sineo1 = 0.0;
    var coseo1 = 0.0;
    for (var ktr = 0; ktr < 10; ktr++) {
        sineo1 = sin(eo1);
        coseo1 = cos(eo1);
        var tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
        tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;
        tem5 = clamp(tem5, -0.95, 0.95);
        eo1 = eo1 + tem5;
        if (abs(tem5) < kKeplerTolerance) {
            break;
        }
    }

    // --- Short period periodics ---

    let ecose = axnl * coseo1 + aynl * sineo1;
    let esine = axnl * sineo1 - aynl * coseo1;
    let el2 = axnl * axnl + aynl * aynl;
    let pl = am * (1.0 - el2);

    if (pl < 0.0) {
        return failed(kErrorNegativeSemiLatusRectum);
    }

    let rl = am * (1.0 - ecose);
    let rdotl = sqrt(am) * esine / rl;
    let rvdotl = sqrt(pl) / rl;
    let betal = sqrt(1.0 - el2);
    temp = esine / (1.0 + betal);
    let sinu = am / rl * (sineo1 - aynl - axnl * temp);
    let cosu = am / rl * (coseo1 - axnl + aynl * temp);
    var su = atan2(sinu, cosu);
    let sin2u = (cosu + cosu) * sinu;
    let cos2u = 1.0 - 2.0 * sinu * sinu;
    temp = 1.0 / pl;
    let temp1 = 0.5 * kJ2 * temp;
    let temp2 = temp1 * temp;

    let mrt = rl * (1.0 - 1.5 * temp2 * betal * e.con41) + 0.5 * temp1 * e.x1mth2 * cos2u;
    su = su - 0.25 * temp2 * e.x7thm1 * sin2u;
    let xnode = nodep + 1.5 * temp2 * cosip * sin2u;
    let xinc = xincp + 1.5 * temp2 * cosip * sinip * cos2u;
    let mvt = rdotl - nm * temp1 * e.x1mth2 * sin2u / kXke;
    let rvdot = rvdotl + nm * temp1 * (e.x1mth2 * cos2u + 1.5 * e.con41) / kXke;

    // --- Orientation vectors ---

    let sinsu = sin(su);
    let cossu = cos(su);
    let snod = sin(xnode);
    let cnod = cos(xnode);
    let sini = sin(xinc);
    let cosi = cos(xinc);
    let xmx = -snod * cosi;
    let xmy = cnod * cosi;
    let uv = vec3f(xmx * sinsu + cnod * cossu, xmy * sinsu + snod * cossu, sini * sinsu);
    let vv = vec3f(xmx * cossu - cnod * sinsu, xmy * cossu - snod * sinsu, sini * cossu);

    var state: PropagatedState;
    state.position = (mrt * uv) * kEarthRadius;
    state.velocity = (mvt * uv + rvdot * vv) * vkmpersec;
    state.padding = 0.0;

    // Below one earth radius, so the orbit has come down. Reported after the position rather than
    // instead of it, because the position is how it was noticed.
    if (mrt < 1.0) {
        state.error = kErrorDecayed;
    } else {
        state.error = kErrorNone;
    }

    return state;
}

@compute @workgroup_size(64) fn computeSGP4(
    @builtin(global_invocation_id) id: vec3u
) {
    let i = id.x;

    // The dispatch is rounded up to whole workgroups, so the tail runs past the end.
    if (i >= arrayLength(&elementSets)) {
        return;
    }

    propagatedStates[i] = propagate(elementSets[i], times[i]);
}
