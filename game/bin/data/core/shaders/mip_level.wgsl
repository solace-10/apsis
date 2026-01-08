struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f
};

@group(0) @binding(0) var ourSampler: sampler;
@group(0) @binding(1) var ourTexture: texture_2d<f32>;

@vertex fn vertexMain(
    @builtin(vertex_index) vertexIndex : u32
) -> VertexOutput
{
    let pos = array(
        // 1st triangle
        vec2f(0.0,  0.0), // center
        vec2f(1.0,  0.0), // right, center
        vec2f(0.0,  1.0), // center, top

        // 2nd triangle
        vec2f(0.0,  1.0), // center, top
        vec2f(1.0,  0.0), // right, center
        vec2f(1.0,  1.0), // right, top
    );

    var out: VertexOutput;
    let xy = pos[vertexIndex];
    out.position = vec4f(xy * 2.0 - 1.0, 0.0, 1.0);
    out.uv = vec2f(xy.x, 1.0 - xy.y);
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    return textureSample(ourTexture, ourSampler, in.uv);
}
