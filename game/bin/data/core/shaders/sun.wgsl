// The Sun: a hard disc at its true angular size, and the glare around it.
//
// The real Sun is 1.5e8 km away and the sector camera's far plane is 200,000 km, so the disc
// cannot be drawn where the Sun is. It does not need to be. At that range the only things an
// observer can measure are a direction and an angular size, and both survive putting a quad a
// fixed distance along the sun direction and sizing it to subtend the right angle.
//
// The direction comes from the global uniforms rather than from a uniform of its own, so the
// disc and the light that shades the planet cannot disagree about where the Sun is: they are the
// same vector. See SunSystem, which is what puts a real ephemeris behind it.
//
// Two fragment entry points over one quad, drawn as two passes:
//
//   discMain     - depth tested, drawn before the planet, so the planet covers it exactly.
//   aureoleMain  - drawn last with the depth test off, because glare is scattering in the
//                  observer's eye and lens rather than an object in the scene. Slicing it along
//                  the planet's silhouette is what it must not do; it is faded instead.

struct VertexOutput
{
    @builtin(position) position: vec4f,
    // Position within the quad, -1 to 1 on each axis. Everything drawn here is a radial function
    // of this, so the fragment stages need nothing else from the geometry.
    @location(0) offset: vec2f,
    // How much of the Sun the planet is leaving uncovered, 0 to 1. Constant over the quad.
    @location(1) visibility: f32
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

// How far in front of the camera the quad sits.
//
// Far enough to clear the planet from anywhere the camera can reach, so that the depth test is
// what hides the disc when the Earth is in front of it: the camera stops at 100,000 km
// (OrbitCameraComponent::maximumDistance in sector.cpp) and the planet reaches 6,378 km past the
// origin, so anything beyond 106,378 km will do. Close enough that the quad's corners stay
// inside the 200,000 km far plane.
const kSunDistance: f32 = 150000.0;

// The Sun subtends 0.533 degrees from Earth, so this is its true angular radius - a disc about
// seven pixels across a 1080p window at the sector camera's 70 degree field of view. Small,
// which is correct. The glare is what makes it read as a light source rather than a dot.
const kSunAngularRadius: f32 = 0.004653;

// How far out the glare is drawn, and with it the size of the quad. Nothing is drawn outside it.
const kGlowAngularRadius: f32 = 0.0698; // 4 degrees, about 15 solar radii

// Peak of the glare, at the centre of the Sun and so underneath the disc.
const kAureoleIntensity: f32 = 0.5;

// Where the glare falls to half its peak, in solar radii. Small numbers give a tight bloom
// hugging the disc, large ones a broad wash.
const kAureoleWidth: f32 = 1.5;

// The equatorial radius the planet mesh is built to - kEarthSemiMajorAxis in
// game/src/space/earth_frame.hpp, which Sector::Initialize() also builds the mesh from. Used
// only to fade the glare as the Sun sets, and a sphere is enough for that: the polar flattening
// is 21 km in 6,378, an order of magnitude inside the softening it is compared against.
const kPlanetRadius: f32 = 6378.137;

// The Sun's distance from the centre of the quad, in units of its own radius: 0 at the centre,
// 1 at the limb, and kGlowAngularRadius / kSunAngularRadius at the edge of the quad.
//
// Everything outside the disc is written in these units rather than in fractions of the quad,
// so the constants above keep their meaning if the quad is ever resized.
fn solarRadii(offset: vec2f) -> f32
{
    return length(offset) * (kGlowAngularRadius / kSunAngularRadius);
}

// How much of the Sun the planet is leaving uncovered, 1 clear to 0 hidden.
//
// The disc does not use this - it is depth tested, which cuts it along the silhouette exactly,
// and that is right for something that really is behind the planet. The glare does, because it
// is not in the scene at all and a hard edge through it reads as a mistake.
fn sunVisibility(cameraPosition: vec3f, sunDirection: vec3f) -> f32
{
    // The planet is at the world origin, so this is the vector from the camera to its centre.
    let toPlanet = -cameraPosition;
    let alongSun = dot(toPlanet, sunDirection);
    if (alongSun <= 0.0)
    {
        // The planet is behind the camera as far as the Sun is concerned. Nothing is in the way.
        return 1.0;
    }

    // How far the line of sight to the Sun passes from the planet's centre.
    let missDistance = length(toPlanet - sunDirection * alongSun);

    // Faded over twice the disc's own radius, projected out to the planet's distance - so the
    // glare dies away over roughly the time the Sun itself takes to set, rather than switching
    // off the instant the centre is covered.
    let softening = 2.0 * kSunAngularRadius * alongSun;
    return smoothstep(kPlanetRadius - softening, kPlanetRadius + softening, missDistance);
}

@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput
{
    let corners = array(
        vec2f(-1.0, -1.0),
        vec2f(1.0, -1.0),
        vec2f(1.0, 1.0),

        vec2f(-1.0, -1.0),
        vec2f(1.0, 1.0),
        vec2f(-1.0, 1.0)
    );
    let corner = corners[vertexIndex];

    // The camera's right and up axes in world space, read out of the view matrix's rotation.
    // Taken from the matrix rather than built from cross products, which would degenerate when
    // the Sun is near the direction the camera is looking or near the pole.
    //
    // Squaring the quad to the screen rather than to the sun direction is what keeps the disc a
    // circle: a screen-aligned quad projects to a rectangle in clip space at any offset from the
    // view axis, whereas one turned to face the Sun would project to a trapezium out at the edge
    // of frame.
    let right = vec3f(uGlobalUniforms.viewMatrix[0][0], uGlobalUniforms.viewMatrix[1][0], uGlobalUniforms.viewMatrix[2][0]);
    let up = vec3f(uGlobalUniforms.viewMatrix[0][1], uGlobalUniforms.viewMatrix[1][1], uGlobalUniforms.viewMatrix[2][1]);

    // directionalLightDirection points towards the light, so it is the sun direction as it
    // stands. Anchoring to the camera rather than to the origin is what makes the distance
    // above a fixed one; the two differ by at most 0.038 degrees of parallax, an eighth of the
    // disc's own radius.
    let sunDirection = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let cameraPosition = uGlobalUniforms.cameraPosition.xyz;
    let center = cameraPosition + sunDirection * kSunDistance;
    let halfExtent = kSunDistance * tan(kGlowAngularRadius);

    let worldPosition = center + (right * corner.x + up * corner.y) * halfExtent;

    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(worldPosition, 1.0);
    out.offset = corner;
    out.visibility = sunVisibility(cameraPosition, sunDirection);
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

// The swap chain is BGRA8Unorm, so each shader encodes its own output. It is the coverage that is
// encoded rather than the final colour: the alpha and the three channels then carry the same
// curve, and the Sun keeps its hue instead of drifting towards white as the encoding lifts its
// dim channels further than its bright one.
fn encodeCoverage(coverage: f32) -> f32
{
    return linearToSrgb(vec3f(clamp(coverage, 0.0, 1.0))).r;
}

@fragment fn discMain(in: VertexOutput) -> @location(0) vec4f
{
    let x = solarRadii(in.offset);

    // Half a pixel either side of the limb. fwidth is the change across a whole pixel, so the
    // obvious smoothstep(edge - fwidth, edge + fwidth, x) ramps over two of them - a third of the
    // diameter of a seven pixel disc. MSAA is no help: it antialiases geometry, and the geometry
    // here is a quad thirty times larger than what is being drawn on it.
    let aa = 0.5 * fwidth(x);
    let disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, x);

    // White, not the light's colour. Every channel of an overexposed source clips, so its core
    // goes white and the tint survives only out in the glare, where the values are low enough to
    // still carry it. Tinting the disc as well makes it read as a coloured object rather than as
    // something too bright to look at.
    let encoded = encodeCoverage(disc);
    return vec4f(vec3f(encoded), encoded);
}

@fragment fn aureoleMain(in: VertexOutput) -> @location(0) vec4f
{
    let r = length(in.offset);
    let x = solarRadii(in.offset);

    // Glare falls off as the inverse square of the angle from the source. That holds for the
    // scattering in an eye, in a lens and in the air, over about three decades, and it is what
    // makes a bright point look bright: a tight bloom on the disc and a long faint tail.
    //
    // A polynomial in the quad's own coordinates cannot do it. Falling off as (1 - r)^n flattens
    // into a plateau at the centre - it was within a factor of 1.6 of the disc six pixels out -
    // which buries the Sun in an even grey ball instead of surrounding it.
    let scaled = x / kAureoleWidth;
    let aureole = kAureoleIntensity / (1.0 + scaled * scaled);

    // An inverse square never reaches zero and the quad has an edge, so the tail is windowed out
    // to meet it. Without this the last tenth of a percent is still a visible straight line.
    let window = pow(max(1.0 - r, 0.0), 2.0);

    let encoded = encodeCoverage(aureole * window * in.visibility);
    return vec4f(uGlobalUniforms.directionalLightColor.rgb * encoded, encoded);
}
