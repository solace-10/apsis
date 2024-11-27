struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) color: vec3f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(1) color: vec4f
};

struct GlobalUniforms
{
    projectionMatrix : mat4x4<f32>,
    viewMatrix : mat4x4<f32>,
    time : f32
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.color = vec4f(in.color, 1.0);
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    return in.color;
}
