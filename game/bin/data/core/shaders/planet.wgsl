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
@group(1) @binding(4) var normalTexture: texture_2d<f32>;
@group(1) @binding(5) var cloudsTexture: texture_2d<f32>;
@group(1) @binding(6) var<uniform> uModelMatrix: mat4x4<f32>;

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

// Relief strength.
const kNormalStrength: f32 = 1.0;

// How fast the cloud deck drifts, in uv per second.
// The current value is significantly faster than in reality.
const kCloudScrollSpeed: f32 = 0.001;

// Overall opacity of the deck.
const kCloudOpacity: f32 = 1.0;

// Cloud albedo.
const kCloudTint = vec3f(1.0, 1.0, 1.0);

// Lifts the cloud terminator past the ground one. Cloud tops are kilometres up,
// so they hold the sun for a while after the surface underneath has lost it,
// and the deck should not end on the same hard line the ground does.
const kCloudTerminatorLift: f32 = 0.15;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;

    // The model matrix carries the planet's spin - the mesh is built about the
    // origin with its prime meridian on +X, and this is what puts that meridian
    // at the current sidereal time. It is a rigid rotation, so the normal takes
    // the same matrix as the position and needs no inverse transpose.
    let worldPos = (uModelMatrix * vec4f(in.position, 1.0)).xyz;

    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(worldPos, 1.0);
    out.worldNormal = (uModelMatrix * vec4f(in.normal, 0.0)).xyz;
    out.uv = in.uv;
    out.worldPos = worldPos;
    return out;
}

// Rebuilds a world space normal from the two channel tangent space map.
//
// Z is not stored. The map keeps only X and Y and Z is recovered from them,
// which saves a channel and guarantees a unit normal rather than trusting one.
// The clamp is belt and braces: this map's X and Y never exceed 0.30, so the
// argument stays near 1, but scaling by kNormalStrength can push it past.
//
// The basis is taken from the surface's own parameterization rather than screen
// space derivatives, which is both cheaper and exact. DirectionToSurfaceUV() in
// game/src/space/earth_frame.cpp builds uv as u = 0.5 - atan2(z, x) / 2pi and
// v = 0.5 - latitude / pi, where latitude is geodetic - so +u runs along
// cross(polar, N) and +v runs south, and south is the direction this map's green
// channel is measured in, so no flip is needed.
//
// Only the sense of v matters here, not its exact form: the ellipsoid's normal
// carries the same 1/a^2 on x and z, so cross(polar, N) is parallel to (z, 0, -x)
// on the spheroid just as it is on a sphere, and any mapping that runs south
// monotonically gives the same basis.
fn perturbNormal(Ngeom: vec3f, uv: vec2f) -> vec3f
{
    let packed = textureSample(normalTexture, textureSampler, uv).rg;
    let xy = (packed * 2.0 - 1.0) * kNormalStrength;
    let z = sqrt(clamp(1.0 - dot(xy, xy), 0.0, 1.0));

    // cross(polar, N) vanishes at the poles, where longitude is degenerate and
    // any tangent will do.
    let polar = vec3f(0.0, 1.0, 0.0);
    let tangentSource = select(polar, vec3f(1.0, 0.0, 0.0), abs(Ngeom.y) > 0.999);
    let T = normalize(cross(tangentSource, Ngeom));
    let B = cross(T, Ngeom);

    return normalize(xy.x * T + xy.y * B + z * Ngeom);
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

    // Two normals, deliberately. The mapped one shades the surface; the geometric
    // one decides where the terminator falls. Driving the day/night split from
    // relief would let a mountain range switch the city lights on and give the
    // terminator a ragged edge that tracks topography rather than the sun.
    let Ngeom = normalize(in.worldNormal);
    let N = perturbNormal(Ngeom, in.uv);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let NdotL = dot(N, L);
    let NdotLGeom = dot(Ngeom, L);
    let diffuse = max(NdotL, 0.0);
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let ambient = uGlobalUniforms.ambientLightColor.rgb;
    let litColor = baseColor * (ambient + lightColor * diffuse);

    // City lights are emissive, so they are added rather than lit - multiplying
    // them by the light would switch them off exactly where they should be seen.
    let nightColor = textureSample(nightTexture, textureSampler, in.uv).rgb;
    let nightEmissive = max(nightColor - kNightBlackPoint, vec3f(0.0));
    let nightFactor = 1.0 - smoothstep(kNightFadeIn, kNightFadeOut, NdotLGeom);

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
    let sunVisibility = smoothstep(0.0, kOceanSunCutoff, NdotLGeom);
    let oceanSpecular = lightColor * pow(NdotH, kOceanShininess) * fresnel
        * oceanMask * kOceanSpecularIntensity * sunVisibility;
    let oceanColorContribution = vec3(0.001, 0.005, 0.012) * oceanMask * sunVisibility;

    let cloudScroll = fract(uGlobalUniforms.time * kCloudScrollSpeed);
    let cloudUv = in.uv - vec2f(cloudScroll, 0.0);
    let cloudAlpha = textureSample(cloudsTexture, textureSampler, cloudUv).r * kCloudOpacity;

    // Clouds are lit from the geometric normal, not the mapped one: the clouds float
    // above the planetary relief.
    let cloudDiffuse = max((NdotLGeom + kCloudTerminatorLift) / (1.0 + kCloudTerminatorLift), 0.0);
    let cloudColor = kCloudTint * (ambient + lightColor * cloudDiffuse) * cloudAlpha;

    // An over operator, written out because the surface contributions are
    // accumulated rather than carried as one value. Everything below the deck is
    // attenuated by it, city lights included - an overcast city is not visible from
    // orbit, and letting the lights through would read as clouds glowing at night.
    let surfaceColor = litColor
        + oceanSpecular + oceanColorContribution
        + baseColor * kNightAmbient * nightFactor
        + nightEmissive * kNightIntensity * nightFactor;

    let finalColor = surfaceColor * (1.0 - cloudAlpha) + cloudColor;

    // Apply gamma correction since swap chain is BGRA8Unorm (not sRGB)
    return vec4f(linearToSrgb(finalColor), 1.0);
}
