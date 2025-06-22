struct VertexInput
{
    @location(0) position: vec3f,
    //@location(1) normal: vec3f,
    //@location(2) uv: vec2f
    @location(3) color: vec3f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f,
    @location(2) uv: vec2f,
    @location(3) color: vec3f
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
    
    var scale: f32 = 1000.0f;
    var cameraOffset: mat4x4<f32> = transpose(mat4x4<f32>(
    	scale, 0.0f, 0.0f, uGlobalUniforms.cameraPosition.x,
    	0.0f, scale, 0.0f, uGlobalUniforms.cameraPosition.y,
    	0.0f, 0.0f, scale, uGlobalUniforms.cameraPosition.z,
    	0.0f, 0.0f, 0.0f, 1.0f
    ));
    
    //uGlobalUniforms.viewMatrix[3];

    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * cameraOffset * uLocalUniforms.modelMatrix * vec4f(in.position, 1.0);
    out.worldPosition = (cameraOffset * uLocalUniforms.modelMatrix * vec4f(in.position, 1.0)).xyz;
    out.worldNormal = vec3f(0.0, 0.0, 1.0);
    out.uv = vec2f(0.0);
    out.color = in.color;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    return vec4f(in.color, 1);
}
