// The orbit of the selected space object, as a ribbon one revolution long centred on where the
// object is now: solid behind it, dashed ahead of it.
//
// There is no vertex buffer. The centreline arrives as a storage buffer and this expands it into a
// triangle strip, two vertices per point, because the expansion has to happen in screen space: a
// ribbon of constant width in kilometres is a bar up close and invisible at the camera's 200,000 km
// limit, and these orbits span both.

struct OrbitPathPoint
{
    position: vec3f, // World space, km.
    arcLength: f32, // km along the path from its first point.
}

struct OrbitPathUniforms
{
    color: vec4f, // rgb, with the alpha the whole ribbon is scaled by.
    halfWidthPixels: f32,
    dashLengthKm: f32,
    anchorArcLength: f32, // Where along the path the object itself is.
    pointCount: u32,
}

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<storage, read> uPoints: array<OrbitPathPoint>;
@group(1) @binding(1) var<uniform> uOrbitPath: OrbitPathUniforms;

const kTwoPi: f32 = 6.283185307179586;

// Half lit, half gap - and what the dashes fade to once they are too small to resolve.
const kDashDutyCycle: f32 = 0.5;

// How much of the revolution each end of the ribbon is faded out over. The path's two ends meet
// behind the object, where a solid edge running into a dashed one reads as a break in the orbit
// rather than as the seam it is. A fraction rather than a distance, so LEO and GEO look the same.
const kFadeFraction: f32 = 0.08;

struct VertexOutput
{
    @builtin(position) position: vec4f,
    // Measured from the object, so its sign says whether a fragment is past or future.
    @location(0) signedArcLength: f32,
    // Zero at whichever end of the path is nearer, one once clear of the fade.
    @location(1) endFade: f32,
}

@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput
{
    let segment = vertexIndex / 2u;
    let side = f32(vertexIndex % 2u) * 2.0 - 1.0;

    let index = min(segment, uOrbitPath.pointCount - 1u);
    let point = uPoints[index];

    // Interior points measure their tangent against the point behind them; the first has nothing
    // behind it and looks forward instead, which is undone below.
    let neighbourIndex = select(index - 1u, index + 1u, index == 0u);
    let neighbour = uPoints[neighbourIndex];

    let viewProjection = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix;
    let clip = viewProjection * vec4f(point.position, 1.0);
    let neighbourClip = viewProjection * vec4f(neighbour.position, 1.0);

    // A point behind the camera has a w at or below zero and would divide into infinities. Only the
    // direction uses the clamped copy; the position emitted below keeps the real w and clips.
    let safeW = max(clip.w, 1e-4);
    let safeNeighbourW = max(neighbourClip.w, 1e-4);

    let viewport = vec2f(uGlobalUniforms.windowWidth, uGlobalUniforms.windowHeight);
    let screen = (clip.xy / safeW) * viewport * 0.5;
    let neighbourScreen = (neighbourClip.xy / safeNeighbourW) * viewport * 0.5;

    var direction = neighbourScreen - screen;

    // Two samples can land on the same pixel, and normalising that is a NaN that would take the
    // whole strip with it.
    if (length(direction) < 1e-6)
    {
        direction = vec2f(1.0, 0.0);
    }
    else
    {
        direction = normalize(direction);
    }

    // Keeps the first point's two vertices on the same sides as the next point's.
    if (index == 0u)
    {
        direction = -direction;
    }

    let perpendicular = vec2f(-direction.y, direction.x);
    let offsetPixels = perpendicular * side * uOrbitPath.halfWidthPixels;

    // Back through the perspective divide: a pixel is 2/viewport of NDC, and clip is NDC times w.
    let offsetClip = offsetPixels * (2.0 / viewport) * clip.w;

    // arcLength is measured from the first point, so it is also the distance to that end.
    let pathLength = uPoints[uOrbitPath.pointCount - 1u].arcLength;
    let distanceToEnd = min(point.arcLength, pathLength - point.arcLength);
    let fadeLength = max(pathLength * kFadeFraction, 1e-5);

    var out: VertexOutput;
    out.position = vec4f(clip.xy + offsetClip, clip.z, clip.w);
    out.signedArcLength = point.arcLength - uOrbitPath.anchorArcLength;
    out.endFade = clamp(distanceToEnd / fadeLength, 0.0, 1.0);
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    // Evaluated for every fragment and selected between afterwards, not computed inside the branch
    // below: fwidth() differences across a quad of fragments, so it must be reached from uniform
    // control flow or the shader will not compile.
    let u = in.signedArcLength / uOrbitPath.dashLengthKm;
    let rate = fwidth(u);

    // cos() rather than fract(), so the pattern has no seam to antialias across.
    let wave = cos(kTwoPi * u);
    let softness = clamp(rate * kTwoPi, 1e-4, 1.0);
    let dash = smoothstep(-softness, softness, wave);

    // Zoomed far enough out a dash is narrower than a pixel, where smoothing only produces stripes
    // that crawl as the camera moves. Past that the pattern gives way to its own average.
    let unresolvable = clamp(rate * 2.0, 0.0, 1.0);
    let dashAlpha = mix(dash, kDashDutyCycle, unresolvable);

    // Where the object has been is solid; where it is going is dashed.
    let alpha = uOrbitPath.color.a * select(1.0, dashAlpha, in.signedArcLength > 0.0) * smoothstep(0.0, 1.0, in.endFade);

    return vec4f(uOrbitPath.color.rgb * alpha, alpha);
}
