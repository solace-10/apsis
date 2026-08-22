// The Sun, as a screen-aligned quad.
//
// The real Sun is 1.5e8 km away and the sector camera's far plane is 200,000 km, so the disc
// cannot be drawn where the Sun is. It does not need to be. At that range the only things an
// observer can measure are a direction and an angular size, and both survive putting the quad a
// fixed distance along the sun direction and sizing it to subtend the right angle.
//
// The direction comes from the global uniforms rather than from a uniform of its own, so the
// disc and the light that shades the planet cannot disagree about where the Sun is: they are the
// same vector. See SunSystem, which is what puts a real ephemeris behind it.

struct VertexOutput
{
    @builtin(position) position: vec4f,
    // Position within the quad, -1 to 1 on each axis. The disc and its halo are both radial
    // functions of this, so the fragment stage needs nothing else.
    @location(0) offset: vec2f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

// How far in front of the camera the quad sits.
//
// Far enough to clear the planet from anywhere the camera can reach, so that the depth test is
// what hides the Sun when the Earth is in front of it: the camera stops at 100,000 km
// (OrbitCameraComponent::maximumDistance in sector.cpp) and the planet reaches 6,378 km past the
// origin, so anything beyond 106,378 km will do. Close enough that the quad's corners stay
// inside the 200,000 km far plane.
const kSunDistance: f32 = 150000.0;

// The Sun subtends 0.533 degrees from Earth, so this is its true angular radius - around 14
// pixels across a 1080p window at the sector camera's 70 degree field of view. Small, which is
// correct; the halo is what makes it read as a light source rather than a dot.
const kSunAngularRadius: f32 = 0.004653;

// The halo, and with it the size of the quad. Nothing is drawn outside it.
const kGlowAngularRadius: f32 = 0.0698; // 4 degrees

// Where the disc ends, as a fraction of the quad's half extent.
const kDiscEdge: f32 = kSunAngularRadius / kGlowAngularRadius;

// How fast the halo falls off. The power is taken on the distance remaining to the edge of the
// quad rather than on the distance from the centre, so the halo reaches exactly zero where the
// geometry does and the quad never shows itself as a square.
const kGlowFalloff: f32 = 4.0;
const kGlowIntensity: f32 = 0.5;

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
    let center = uGlobalUniforms.cameraPosition.xyz + sunDirection * kSunDistance;
    let halfExtent = kSunDistance * tan(kGlowAngularRadius);

    let worldPosition = center + (right * corner.x + up * corner.y) * halfExtent;

    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(worldPosition, 1.0);
    out.offset = corner;
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
    let r = length(in.offset);

    // The disc is a dozen pixels across, so its edge has to be resolved analytically - MSAA only
    // antialiases geometry, and the geometry here is a quad many times larger. fwidth gives the
    // change in r over one pixel, which makes the transition exactly one pixel wide however far
    // the camera has zoomed out.
    let pixel = fwidth(r);
    let disc = 1.0 - smoothstep(kDiscEdge - pixel, kDiscEdge + pixel, r);

    let glow = kGlowIntensity * pow(max(1.0 - r, 0.0), kGlowFalloff);
    let coverage = clamp(disc + glow, 0.0, 1.0);

    // Gamma, as everywhere else - the swap chain is BGRA8Unorm, so each shader encodes its own
    // output. It is the coverage that is encoded rather than the final colour: alpha and the
    // three channels then carry the same curve, which is what the premultiplied blend needs, and
    // the Sun keeps its warmth instead of drifting white as the encoding lifts its dim channels
    // further than its bright one.
    let encodedCoverage = linearToSrgb(vec3f(coverage)).r;

    let sunColor = uGlobalUniforms.directionalLightColor.rgb;
    return vec4f(sunColor * encodedCoverage, encodedCoverage);
}
