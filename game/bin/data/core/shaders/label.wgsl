struct VertexInput
{
    @location(0) position: vec2f,
    @location(1) color: vec4f,
    @location(2) uv: vec2f
};

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(1) color: vec4f,
    @location(2) uv: vec2f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var uSampler: sampler;
@group(1) @binding(1) var uTexture: texture_2d<f32>;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    let x = ((2.0 * in.position.x) / uGlobalUniforms.windowWidth) - 1.0;
    let y = 1.0 - ((2.0 * in.position.y) / uGlobalUniforms.windowHeight);
    out.position = vec4f(x, y, 0.0, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    let texelSize = 1.0 / vec2f(textureDimensions(uTexture, 0));
    let glyphAlpha = textureSample(uTexture, uSampler, in.uv).a;

    // Dilate the glyph by one texel to form the outline. The bitmap font has one texel
    // of padding around every glyph, so this stays within the glyph's own quad and can
    // never pick up a neighbouring glyph from the atlas.
    //
    // A cross rather than a full 3x3: at this font size the diagonal taps thicken the
    // strokes enough to start closing the counters of characters such as '6' and 'A',
    // and the atlas is anti-aliased, so the axis-aligned taps already cover diagonal
    // edges well enough.
    const kOutlineOffsets = array<vec2f, 4>(
        vec2f(-1.0, 0.0),
        vec2f(1.0, 0.0),
        vec2f(0.0, -1.0),
        vec2f(0.0, 1.0)
    );

    var dilated = glyphAlpha;
    for (var i = 0; i < 4; i++)
    {
        let offset = kOutlineOffsets[i] * texelSize;
        dilated = max(dilated, textureSample(uTexture, uSampler, in.uv + offset).a);
    }

    // Source-over composite of the glyph on top of the outline.
    let outlineColor = vec3f(0.0);
    let outlineAlpha = dilated * (1.0 - glyphAlpha);
    let alpha = glyphAlpha + outlineAlpha;
    // Guarded against a zero divide: fully transparent fragments would otherwise
    // produce a NaN colour, which survives the blend as NaN rather than as nothing.
    let rgb = (in.color.rgb * glyphAlpha + outlineColor * outlineAlpha) / max(alpha, 0.00001);
    return vec4f(rgb, alpha);
}
