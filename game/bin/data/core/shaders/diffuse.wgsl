struct GlobalUniforms
{
    projectionMatrix : mat4x4<f32>,
    viewMatrix : mat4x4<f32>,
    modelMatrix : mat4x4<f32>,
    time : f32
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@vertex fn vertexMain(@location(0) in_vertex_position: vec3f) -> @builtin(position) vec4f 
{
    return vec4f(in_vertex_position, 1);
}

@fragment fn fragmentMain() -> @location(0) vec4f 
{
    return vec4f(1, cos(uGlobalUniforms.time) / 2.0 + 0.5, 0, 1);
}