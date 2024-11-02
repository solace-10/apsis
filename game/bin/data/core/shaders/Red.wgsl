struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) normalVS: vec4f // vertex normal in view space
};

struct GlobalUniforms
{
    projectionMatrix : mat4x4<f32>,
    viewMatrix : mat4x4<f32>,
    time : f32
};

struct LocalUniforms
{
    modelMatrix : mat4x4<f32>
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<uniform> uLocalUniforms: LocalUniforms;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * uLocalUniforms.modelMatrix * vec4f(in.position, 1.0);
    out.normalVS = uGlobalUniforms.viewMatrix * uLocalUniforms.modelMatrix * vec4f(in.normal,1.0);
    return out;
}

@fragment fn fragmentMain() -> @location(0) vec4f 
{
    return vec4f(1, cos(uGlobalUniforms.time) / 2.0 + 0.5, 0, 1);
}
