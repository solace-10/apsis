struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f
};

struct AtmosphereUniforms
{
    color: vec3f,      // offset 0
    height: f32,       // offset 12
    density: f32,      // offset 16
    _padding0: f32,    // offset 20
    _padding1: f32,    // offset 24
    _padding2: f32     // offset 28
    // Total: 32 bytes
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<uniform> uAtmosphere: AtmosphereUniforms;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    // Expand vertex outward along normal by atmosphere height
    let expandedPosition = in.position + in.normal * uAtmosphere.height;
    
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(expandedPosition, 1.0);
    out.worldPosition = expandedPosition;
    out.worldNormal = in.normal;
    return out;
}

// Convert linear color to sRGB (gamma correction)
fn linearToSrgb(linear: vec3f) -> vec3f 
{
    let cutoff = linear < vec3f(0.0031308);
    let higher = vec3f(1.055) * pow(linear, vec3f(1.0/2.4)) - vec3f(0.055);
    let lower = linear * vec3f(12.92);
    return select(higher, lower, cutoff);
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    let N = normalize(in.worldNormal);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let V = normalize(uGlobalUniforms.cameraPosition.xyz - in.worldPosition);
    
    // Fresnel-like effect: atmosphere is more visible at the limb (edges)
    // When view direction is perpendicular to normal, we see more atmosphere
    let NdotV = abs(dot(N, V));
    let limb = 1.0 - NdotV;
    let limbIntensity = pow(limb, uAtmosphere.density);
    
    // Light scattering: atmosphere glows on the day side
    let NdotL = dot(N, L);
    // Soften the day/night transition for atmospheric scattering
    let scatter = smoothstep(-0.2, 0.5, NdotL);
    
    // Combine limb darkening with light scattering
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let atmosphereColor = uAtmosphere.color * lightColor * scatter;
    
    // Final alpha based on limb intensity
    let alpha = limbIntensity * scatter * 0.8;
    
    return vec4f(linearToSrgb(atmosphereColor), alpha);
}
