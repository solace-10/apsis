struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldNormal: vec3f,
    @location(1) uv: vec2f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var colorTexture: texture_2d<f32>;
@group(1) @binding(2) var nightTexture: texture_2d<f32>;

// The band of dot(N, L) over which city lights fade. They are fully out by the
// time a point is facing the sun at all, and reach full strength a little past
// the terminator - roughly where twilight gives way to night, since cities are
// not visible against a sky that is still lit.
const kNightFadeIn: f32 = -0.25;
const kNightFadeOut: f32 = 0.0;

// The night map is a render rather than an emissive mask, and its unlit areas
// carry a blue wash that tracks ground albedo - Greenland and the Sahara sit at
// 2.5x mid-ocean. That is baked moonlight, not emission, and it has no business
// being added: it asserts a moon this scene does not have, in a colour moonlight
// is not. Moonlight is sunlight off a red-brown regolith and comes back warmer
// than the sun; the belief that it is blue is the Purkinje effect in a dark
// adapted eye, not a property of the light.
//
// Subtracting the floor per channel leaves only what stands above it. The values
// are the brightest background measured in the map, so ocean clamps to zero. It
// costs Tokyo 2% of its red and 18% of its blue, which if anything moves the
// lights closer to the sodium and LED sources they represent.
const kNightBlackPoint = vec3f(0.004, 0.005, 0.013);

// Scales the city lights once the floor is gone. Only 0.15% of the map is above
// 0.2, so the dark side stays dark and the city cores carry it.
const kNightIntensity: f32 = 1.0;

// Night side illumination, kept separate from the map so that it multiplies the
// real albedo: bright ground is then bright because it is bright, not because a
// texture says so, and the colour is a choice rather than an inheritance. Zero
// is the honest default - a moonless night side from orbit is black apart from
// the cities. A dim neutral value reads as moonlight; a blue one is a stylistic
// decision, which is fine as long as it is made here and deliberately.
const kNightAmbient = vec3f(0.0, 0.0, 0.0);

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.worldNormal = in.normal;
    out.uv = in.uv;
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
    let baseColor = textureSample(colorTexture, textureSampler, in.uv).rgb;
    let N = normalize(in.worldNormal);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let NdotL = dot(N, L);
    let diffuse = max(NdotL, 0.0);
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let ambient = uGlobalUniforms.ambientLightColor.rgb;
    let litColor = baseColor * (ambient + lightColor * diffuse);

    // City lights are emissive, so they are added rather than lit - multiplying
    // them by the light would switch them off exactly where they should be seen.
    let nightColor = textureSample(nightTexture, textureSampler, in.uv).rgb;
    let nightEmissive = max(nightColor - kNightBlackPoint, vec3f(0.0));
    let nightFactor = 1.0 - smoothstep(kNightFadeIn, kNightFadeOut, NdotL);

    let finalColor = litColor
        + baseColor * kNightAmbient * nightFactor
        + nightEmissive * kNightIntensity * nightFactor;

    // Apply gamma correction since swap chain is BGRA8Unorm (not sRGB)
    return vec4f(linearToSrgb(finalColor), 1.0);
}
