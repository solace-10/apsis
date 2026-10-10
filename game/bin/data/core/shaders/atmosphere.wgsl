// Sean O'Neil's Atmospheric Scattering (adapted for atmosphere shell rendering)
// Reference: GPU Gems 2, Chapter 16 - Accurate Atmospheric Scattering
// 
// Ray marching is done per-pixel in the fragment shader for accuracy.

struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPos: vec3f  // World position on atmosphere shell
};

struct AtmosphereUniforms
{
    v3InvWavelength: vec3f,
    fInnerRadius: f32,

    fInnerRadius2: f32,
    fOuterRadius: f32,
    fOuterRadius2: f32,
    fKrESun: f32,

    fKmESun: f32,
    fKr4PI: f32,
    fKm4PI: f32,
    fScale: f32,

    fScaleDepth: f32,
    fScaleOverScaleDepth: f32,
    g: f32,
    g2: f32,

    fSamples: f32,
    fAtmosphereHeight: f32,
    _padding0: f32,
    _padding1: f32
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<uniform> uAtmosphere: AtmosphereUniforms;

// Fades scattering out across the terminator. Off, and superseded by the shadow
// test below - but kept, because what it was masking is now known and it is worth
// having the record next to the switch.
//
// It was hiding a blue veil across the whole night side. O'Neil's model has no
// occlusion test: scale() approximates optical depth along the light ray, which
// is a different question from whether that ray is blocked. scale() does return
// 45 for a sample over the night side, but it is multiplied by fDepth, which is
// exp(-4) at the top of the shell, so a high sample sitting in full shadow kept
// 59% of its blue. Summed across the disc, that was the veil.
//
// The fade suppressed it by light angle alone, which cannot tell a shadowed
// sample from a merely grazing one, so it took the sunset with it. Warm limb
// colour needs fScatter around 6 to 10, because Rayleigh's 1/lambda^4 biases
// scattering 3.5x towards blue and extinction has to overcome that before red
// wins - putting the band at fLightAngle between roughly -0.05 and -0.1, exactly
// where this fade multiplies by 0.32 and 0.16.
//
// getLightVisibility asks the occlusion question directly, so it removes the veil
// without touching the band. This should not need to come back.
const kEnableTerminatorFade: bool = false;

// Softness of the shadow edge, as a fraction of the planet radius. The sun is
// half a degree wide rather than a point, so the umbra has a real penumbra; this
// also stops the boundary aliasing into a hard line along the terminator.
const kShadowSoftness: f32 = 0.01;

// O'Neil's scale function - approximates optical depth integral
// Input fCos should be clamped to valid range
fn scale(fCos: f32) -> f32 
{
    let x = 1.0 - fCos;
    return uAtmosphere.fScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

// How much sunlight reaches a point: 0 deep inside the planet's shadow, 1 outside
// it, with a soft edge between.
//
// For a directional light the umbra is a cylinder of the planet's radius pointing
// away from the sun, so this is a side test plus a distance from the shadow axis.
// The common case - anything sunward of the terminator plane - returns before the
// square root.
//
// This is what separates a shadowed sample from a grazing one, which is the
// distinction the old terminator fade could not make. A sample low over the night
// side but only just past the terminator sits on the boundary rather than inside
// it, and keeps its long, reddened light path. A sample high above the night side
// is genuinely lit, which is the twilight arc that photographs of the limb show.
fn getLightVisibility(v3Point: vec3f, v3LightDir: vec3f) -> f32
{
    // How far past the terminator plane this point can travel before the planet
    // rises over its horizon: sqrt(r^2 - R^2), which is zero at the surface and
    // grows with altitude. That single term is what keeps the upper atmosphere
    // lit above the night side and gives the twilight arc its shape.
    let fHorizon = sqrt(max(0.0, dot(v3Point, v3Point) - uAtmosphere.fInnerRadius2));
    let fAxisDistance = dot(v3Point, v3LightDir);
    let fSoftness = uAtmosphere.fInnerRadius * kShadowSoftness;

    // Measured along the light axis rather than perpendicular to it. Both express
    // the same cylinder, but the transition happens along this axis, so softening
    // the perpendicular distance instead fades only the night side of the boundary
    // and leaves a step at the terminator plane - at the surface, where the
    // perpendicular distance equals R exactly, that step was 1.0 to 0.5.
    return smoothstep(-fHorizon - fSoftness, -fHorizon + fSoftness, fAxisDistance);
}

// Ray-sphere intersection - returns distance to near intersection (entering the sphere)
// Returns negative value if no intersection
fn getNearIntersection(v3Pos: vec3f, v3Ray: vec3f, fRadius2: f32) -> f32 
{
    let B = 2.0 * dot(v3Pos, v3Ray);
    let C = dot(v3Pos, v3Pos) - fRadius2;
    let fDet = B * B - 4.0 * C;
    if (fDet < 0.0) {
        return -1.0;
    }
    return 0.5 * (-B - sqrt(fDet));
}

// Ray-sphere intersection - returns distance to far intersection (exiting the sphere)
fn getFarIntersection(v3Pos: vec3f, v3Ray: vec3f, fRadius2: f32) -> f32 
{
    let B = 2.0 * dot(v3Pos, v3Ray);
    let C = dot(v3Pos, v3Pos) - fRadius2;
    let fDet = B * B - 4.0 * C;
    if (fDet < 0.0) {
        return -1.0;
    }
    return 0.5 * (-B + sqrt(fDet));
}

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    // Expand vertex to atmosphere's outer edge
    let v3Pos = in.position + in.normal * uAtmosphere.fAtmosphereHeight;
    
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(v3Pos, 1.0);
    out.worldPos = v3Pos;
    
    return out;
}

fn getMiePhase(fCos: f32, g: f32, g2: f32) -> f32
{
    let fCos2 = fCos * fCos;
    return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos2) / pow(1.0 + g2 - 2.0 * g * fCos, 1.5);
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    let v3CameraPos = uGlobalUniforms.cameraPosition.xyz;
    let v3LightDir = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let v3Pos = in.worldPos;
    
    // Ray direction from camera toward this fragment (into the atmosphere)
    let v3Ray = normalize(v3Pos - v3CameraPos);
    
    // Check if ray hits the planet surface (inner sphere)
    let fPlanetHit = getNearIntersection(v3Pos, v3Ray, uAtmosphere.fInnerRadius2);
    
    // Determine how far to ray march:
    // - If ray hits planet (fPlanetHit > 0), march to the planet surface
    // - Otherwise, ray grazes the limb - find where it exits the outer atmosphere
    var fRayLength: f32;
    if (fPlanetHit > 0.0) 
    {
        fRayLength = fPlanetHit;
    } 
    else
    {
        // Ray exits on far side of outer atmosphere
        fRayLength = getFarIntersection(v3Pos, v3Ray, uAtmosphere.fOuterRadius2);
        fRayLength = max(0.0, fRayLength);
    }
    
    // Starting optical depth at the outer atmosphere edge
    let fStartHeight = length(v3Pos);
    let fStartAngle = clamp(dot(v3Ray, v3Pos) / fStartHeight, -1.0, 1.0);
    let fStartDepth = exp(uAtmosphere.fScaleOverScaleDepth * (uAtmosphere.fInnerRadius - fStartHeight));
    let fStartOffset = fStartDepth * scale(fStartAngle);

    // Ray marching setup
    // fSampleLength is the actual distance step along the ray
    // This is what we multiply by to approximate the integral
    let nSamples = i32(uAtmosphere.fSamples);
    let fSampleLength = fRayLength / uAtmosphere.fSamples;
    let v3SampleRay = v3Ray * fSampleLength;
    var v3SamplePoint = v3Pos + v3SampleRay * 0.5;

    // Accumulate scattering along the ray through the atmosphere
    // This is a numerical integration: ∫ density * attenuation * ds
    // Each sample contributes: value_at_sample * segment_length
    var v3FrontColor = vec3f(0.0, 0.0, 0.0);
    for (var i = 0; i < nSamples; i = i + 1)
    {
        let fHeight = length(v3SamplePoint);
        
        let fDepth = exp(uAtmosphere.fScaleOverScaleDepth * (uAtmosphere.fInnerRadius - fHeight));
        
        // Optical depth for light reaching this point and then reaching the camera
        let fLightAngle = dot(v3LightDir, v3SamplePoint) / fHeight;
        let fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
        let fScatter = max(0.0, fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)));
        
        // Smooth terminator falloff - fade scattering as we move into the night side
        // fLightAngle of 0 = terminator, negative = night side
        // Fade from full (1.0) at terminatorStart to zero at terminatorEnd
        var terminatorFade = 1.0;
        if (kEnableTerminatorFade)
        {
            let terminatorStart = 0.2;  // Start fading slightly before terminator (~12 degrees)
            let terminatorEnd = -0.2;   // Fully dark past terminator (~12 degrees into night)
            terminatorFade = smoothstep(terminatorEnd, terminatorStart, fLightAngle);
        }

        // Attenuation due to out-scattering along the path
        let v3Attenuate = exp(-fScatter * (uAtmosphere.v3InvWavelength * uAtmosphere.fKr4PI + uAtmosphere.fKm4PI));
        
        // Samples the planet is standing in front of receive no sunlight, so they
        // scatter none. Without this the night side keeps a blue veil, because the
        // optical depth approximation alone never gets large enough at altitude to
        // extinguish it.
        let fLightVisibility = getLightVisibility(v3SamplePoint, v3LightDir);

        // Accumulate: density * attenuation * path_segment_length * scale_factor
        // The scale factor normalizes the path length to atmosphere thickness units
        v3FrontColor = v3FrontColor + v3Attenuate * fDepth * fSampleLength * uAtmosphere.fScale * terminatorFade * fLightVisibility;
        v3SamplePoint = v3SamplePoint + v3SampleRay;
    }

    // Calculate Rayleigh and Mie colors
    let v3RayleighColor = v3FrontColor * (uAtmosphere.v3InvWavelength * uAtmosphere.fKrESun);
    let v3MieColor = v3FrontColor * uAtmosphere.fKmESun;
    
    // Apply phase functions
    let v3Direction = normalize(v3CameraPos - v3Pos);
    let fCos = dot(v3LightDir, v3Direction);
    let mie = getMiePhase(fCos, uAtmosphere.g, uAtmosphere.g2) * v3MieColor;
    
    var color = v3RayleighColor + mie;

    // Alpha is how much of the planet the air in front of it hides: one minus the
    // transmittance along the view ray. That depends on how much air the ray
    // crosses, not on how bright the sun is, so ESun brightens the haze without
    // thickening it. Transmittance is per channel, but one alpha can carry only its
    // luminance; per channel would need dual-source blending. The blend is
    // premultiplied, so the scattering adds at full strength regardless.
    // Rays that miss the planet have only space behind them.
    var alpha = 0.0;
    if (fPlanetHit > 0.0)
    {
        let v3Ground = v3Pos + v3Ray * fRayLength;
        let fGroundDepth = exp(uAtmosphere.fScaleOverScaleDepth * (uAtmosphere.fInnerRadius - length(v3Ground)));
        let fGroundAngle = clamp(dot(-v3Ray, normalize(v3Ground)), 0.0, 1.0);
        let fTopAngle = clamp(dot(-v3Ray, v3Pos) / fStartHeight, 0.0, 1.0);
        let fOpticalDepth = max(0.0, fGroundDepth * scale(fGroundAngle) - fStartDepth * scale(fTopAngle));
        let v3Transmittance = exp(-fOpticalDepth * (uAtmosphere.v3InvWavelength * uAtmosphere.fKr4PI + uAtmosphere.fKm4PI));
        alpha = 1.0 - dot(v3Transmittance, vec3f(0.2126, 0.7152, 0.0722));
    }

    return vec4f(color, alpha);
}
