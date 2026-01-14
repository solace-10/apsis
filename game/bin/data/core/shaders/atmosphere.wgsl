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

// O'Neil's scale function - approximates optical depth integral
// Input fCos should be clamped to valid range
fn scale(fCos: f32) -> f32 
{
    let x = 1.0 - fCos;
    return uAtmosphere.fScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
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
        let terminatorStart = 0.2;  // Start fading slightly before terminator (~12 degrees)
        let terminatorEnd = -0.2;   // Fully dark past terminator (~12 degrees into night)
        let terminatorFade = smoothstep(terminatorEnd, terminatorStart, fLightAngle);
        
        // Attenuation due to out-scattering along the path
        let v3Attenuate = exp(-fScatter * (uAtmosphere.v3InvWavelength * uAtmosphere.fKr4PI + uAtmosphere.fKm4PI));
        
        // Accumulate: density * attenuation * path_segment_length * scale_factor
        // The scale factor normalizes the path length to atmosphere thickness units
        v3FrontColor = v3FrontColor + v3Attenuate * fDepth * fSampleLength * uAtmosphere.fScale * terminatorFade;
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
    let luminance = dot(color, vec3f(0.299, 0.587, 0.114));
    let alpha = clamp(luminance * 2.0, 0.0, 1.0);

    return vec4f(color, alpha);
}
