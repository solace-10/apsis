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
@group(2) @binding(0) var defaultSampler: sampler;
@group(2) @binding(1) var baseTexture: texture_2d<f32>;
//@group(2) @binding(2) var metallicRoughnessTexture: texture_2d<f32>;
//@group(2) @binding(3) var normalTexture: texture_2d<f32>;
//@group(2) @binding(4) var occlusionTexture: texture_2d<f32>;
//@group(2) @binding(5) var emissiveTexture: texture_2d<f32>;

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
	
	let NdotL = dot(in.worldNormal, lightDir);
	
	var diffuseStrength = 0.0;
	if (NdotL >= 0.0)
	{
		diffuseStrength = 1.0;
	}
	//diffuseStrength = max(NdotL, 0.0);
	let ambientStrength = vec3f(0.2);
	
	let viewDir = uGlobalUniforms.viewMatrix[0].xyz;
	let rimDot = max(1 - dot(viewDir, in.worldNormal), 0);
	let rimAmount = 0.7;
	let rimIntensity = smoothstep(rimAmount - 0.01, rimAmount + 0.01, rimDot);
	//let rim = rimIntensity;
	let rim = 0.0;
	
	let objectColor = textureSample(baseTexture, defaultSampler, in.uv).rgb;
	//return vec4f(in.worldNormal, 1.0);
    return vec4f(objectColor * (ambientStrength + diffuseStrength + rim), 1);
}
