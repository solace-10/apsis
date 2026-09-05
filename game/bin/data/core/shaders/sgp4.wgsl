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

// WGS72, the model TLEs are fitted with. Written out to the precision of the double the CPU side
// uses, so that the compiler rounds the same value rather than rounding a different one.
const kEarthRadius: f32 = 6378.135;
const kXke: f32 = 0.074366916133173422;
const kJ2: f32 = 0.001082616;

const kTwoPi: f32 = 6.283185307179586;
const kTwoThirds: f32 = 0.6666666666666666;

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

fn propagate(e: SGP4ElementSet, t: f32) -> PropagatedState {
    // Refused rather than attempted, exactly as the CPU version does: a deep-space element set was
    // never initialised, so no_unkozai is zero and the first thing this would do is divide by it.
    if (e.deepSpace != 0u) {
        return failed(kErrorDeepSpaceNotSupported);
    }

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
    let inclm = e.inclo;

    if (nm <= 0.0) {
        return failed(kErrorMeanMotionNotPositive);
    }

    let am = pow((kXke / nm), kTwoThirds) * tempa * tempa;
    nm = kXke / pow(am, 1.5);
    em = em - tempe;

    if ((em >= 1.0) || (em < -0.001)) {
        return failed(kErrorMeanElementsOutOfRange);
    }

    em = max(em, 1.0e-6);

    mm = mm + e.no_unkozai * templ;
    var xlm = mm + argpm + nodem;

    nodem = nodem % kTwoPi;
    argpm = argpm % kTwoPi;
    xlm = xlm % kTwoPi;
    mm = (xlm - argpm - nodem) % kTwoPi;

    let sinip = sin(inclm);
    let cosip = cos(inclm);

    // Where the deep-space path would add the lunar-solar periodics.
    let ep = em;
    let xincp = inclm;
    let argpp = argpm;
    let nodep = nodem;
    let mp = mm;

    // --- Long period periodics ---

    let axnl = ep * cos(argpp);
    var temp = 1.0 / (am * (1.0 - ep * ep));
    let aynl = ep * sin(argpp) + temp * e.aycof;
    let xl = mp + argpp + nodep + temp * e.xlcof * axnl;

    // --- Kepler's equation ---
    //
    // The CPU version stops when the correction falls below 1e-12, which f32 cannot reach: the
    // loop here always runs all ten passes. That is bounded and it converges long before the tenth,
    // so the cost is wasted iterations rather than accuracy - but it is why there is no early out.
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
