
struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) color: vec4f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) color: vec4f,
    @location(2) uv: vec2f
};

struct LocalUniforms
{
    modelMatrix: mat4x4<f32>
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var defaultSampler: sampler;
@group(1) @binding(1) var baseTexture: texture_2d<f32>;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.worldPosition = in.position;
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    return in.color * textureSample(baseTexture, defaultSampler, in.uv).r;
}
