struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f,
    @location(2) uv: vec2f
};

struct GlobalUniforms
{
    projectionMatrix: mat4x4<f32>,
    viewMatrix: mat4x4<f32>,
    time: f32
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
    out.worldPosition = (uLocalUniforms.modelMatrix * vec4f(in.position, 1.0)).xyz;
    out.worldNormal = (uLocalUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
    out.uv = in.uv;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
	let lightPosition = vec3f(100.0, 100.0, 100.0);
	let lightDir = normalize(lightPosition - in.worldPosition);
	let diffuseStrength = max(dot(in.worldNormal, lightDir), 0.0);
	let ambientStrength = vec3f(0.2);
	let objectColor = vec3f(0.0, 0.2, 0.5);
	//return vec4f(in.worldNormal, 1.0);
    return vec4f(objectColor * ambientStrength + objectColor * diffuseStrength, 1);
}
