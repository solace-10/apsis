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
    @location(1) uv: vec2f,
    @location(2) worldPos: vec3f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var colorTexture: texture_2d<f32>;
@group(1) @binding(2) var nightTexture: texture_2d<f32>;
@group(1) @binding(3) var specularTexture: texture_2d<f32>;

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

// Ocean specular. The specular map is a single channel land/sea mask - 1 over
// water, 0 over land - so the highlight is confined to water without needing to
// know anything else about the surface.
//
// kOceanF0 is the reflectance of a water/air interface at normal incidence.
// 0.02 is the physical value: water is a poor mirror seen face on, which is why
// the glint only really appears as the geometry turns away.
const kOceanF0: f32 = 0.02;

// The Schlick exponent that carries reflectance from F0 up towards 1 at grazing
// angles. 5.0 is the physical value; lowering it broadens the glint across more
// of the disc, which is the main dial for making the effect obvious rather than
// merely correct.
const kOceanFresnelPower: f32 = 5.0;

// How tight the sun's reflection is. Higher is a smaller, harder highlight - a
// glassier sea - and lower spreads it into the choppier smear a real ocean gives.
const kOceanShininess: f32 = 100.0;

// Overall strength, applied last, for when the physical result is not the result
// that looks right.
const kOceanSpecularIntensity: f32 = 1.0;

// How sharply the highlight is cut off at the terminator. Nothing reflects a sun
// that has set, and this keeps the ocean from glinting on the night side at all.
const kOceanSunCutoff: f32 = 0.05;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.worldNormal = in.normal;
    out.uv = in.uv;
    // The mesh is built in world space and drawn without a model transform, so
    // the incoming position is already what the view vector needs.
    out.worldPos = in.position;
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

    // Sun glint off water. The half vector formulation puts the Fresnel term on
    // the microfacets actually reflecting towards the camera, rather than on the
    // surface normal, which is what makes the highlight brighten and spread as it
    // approaches the limb instead of just sitting where the sun is.
    let V = normalize(uGlobalUniforms.cameraPosition.xyz - in.worldPos);
    let H = normalize(L + V);
    let VdotH = max(dot(V, H), 0.0);
    let NdotH = max(dot(N, H), 0.0);

    let fresnel = kOceanF0 + (1.0 - kOceanF0) * pow(1.0 - VdotH, kOceanFresnelPower);
    let oceanMask = textureSample(specularTexture, textureSampler, in.uv).r;
    let sunVisibility = smoothstep(0.0, kOceanSunCutoff, NdotL);
    let oceanSpecular = lightColor * pow(NdotH, kOceanShininess) * fresnel
        * oceanMask * kOceanSpecularIntensity * sunVisibility;
    let oceanColorContribution = vec3(0.001, 0.005, 0.012) * oceanMask * sunVisibility;

    let finalColor = litColor
        + oceanSpecular + oceanColorContribution
        + baseColor * kNightAmbient * nightFactor
        + nightEmissive * kNightIntensity * nightFactor;

    // Apply gamma correction since swap chain is BGRA8Unorm (not sRGB)
    return vec4f(linearToSrgb(finalColor), 1.0);
}
