struct VertexInput
{
    @builtin(instance_index) instanceIdx: u32,
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f,
    @location(2) normal: vec3f,
    @location(3) uv: vec2f
};

struct LocalUniforms
{
    modelMatrix: mat4x4<f32>
};

struct InstanceUniforms
{
    transform: array<mat4x4<f32>, 256>
};

// The dynamic uniforms are packed into an array of 8 vec4fs.
// We can access them through their location in the array, e.g. params[0].x for the first float.
//
// struct DynamicUniforms {
//    params: array<vec4f, 8>,
// }
//
// However, WGSL actually does allow us to access the raw buffer in a structured way, even if we
// explicitly define it in code:

struct DynamicUniforms {
    shieldDamage: f32,
    shieldPower: f32
}

// Hexagon pattern constants
const hexS = vec2f(1.0, 1.7320508); // sqrt(3)

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<uniform> uLocalUniforms: LocalUniforms;
@group(2) @binding(0) var<uniform> uInstanceUniforms: InstanceUniforms;
@group(3) @binding(0) var defaultSampler: sampler;
@group(3) @binding(1) var baseTexture: texture_2d<f32>;
//@group(3) @binding(2) var metallicRoughnessTexture: texture_2d<f32>;
//@group(3) @binding(3) var normalTexture: texture_2d<f32>;
//@group(3) @binding(4) var occlusionTexture: texture_2d<f32>;
//@group(3) @binding(5) var emissiveTexture: texture_2d<f32>;
@group(3) @binding(6) var<uniform> uDynamicUniforms: DynamicUniforms;

// Hexagon pattern functions (ported from hex.glsl)
fn calcHexDistance(p: vec2f) -> f32
{
    let absP = abs(p);
    return max(dot(absP, hexS * 0.5), absP.x);
}

fn calcHexOffset(uv: vec2f) -> vec2f
{
    let hexCenter = round(vec4f(uv, uv - vec2f(0.5, 1.0)) / vec4f(hexS.xyxy));
    let offset = vec4f(uv - hexCenter.xy * hexS, uv - (hexCenter.zw + 0.5) * hexS);
    return select(offset.zw, offset.xy, dot(offset.xy, offset.xy) < dot(offset.zw, offset.zw));
}

fn smoothStep(edge0: f32, edge1: f32, x: f32) -> f32
{
    let t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

fn hexSmoothstep(resolution: f32, value: f32, thickness: f32) -> f32
{
    return smoothStep(thickness / resolution, 0.0, abs(value));
}

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    let instancePos = vec3f(25.0 * f32(in.instanceIdx), 0.0, 0.0);
    let modelMatrix = uInstanceUniforms.transform[in.instanceIdx] * uLocalUniforms.modelMatrix;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * modelMatrix * vec4f(in.position, 1.0);
    out.worldPosition = (modelMatrix * vec4f(in.position, 1.0)).xyz;
    out.worldNormal = (modelMatrix * vec4f(in.normal, 0.0)).xyz;
    out.normal = in.normal;
    out.uv = in.uv;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    let jitterScale = 0.0025;
    let jitter = array<vec2f, 4>(
       vec2f(-1.0, -1.0) * jitterScale,
       vec2f(-1.0, 1.0) * jitterScale,
       vec2f(1.0, 1.0) * jitterScale,
       vec2f(1.0, -1.0) * jitterScale
    );

    var ringsPattern = vec3f(0.0);
    let time = uGlobalUniforms.time * 1.5;
    let resolution = 128.0;

    for (var i = 0; i < 4; i++)
    {
        let generatedUV = vec2(in.normal.x + 0.5 + jitter[i].x, in.normal.y + 0.5 + jitter[i].y);
        let uv = generatedUV * 4.0;

        let uvLength = length(generatedUV * 2.0 - 1.0);
        let animationPhase = cos(2.0 * (2.0 * uvLength - time));
        let hexDist = calcHexDistance(calcHexOffset(uv));

        ringsPattern += hexSmoothstep(resolution, abs(sin(hexDist * animationPhase * 10.0)), 18.0);
    }

    let ringsColor = ringsPattern / 4.0 * vec3f(0.5, 1.0, 1.0);
    let finalColor = ringsColor + vec3(0.0f, 0.25f, 0.5f);

	// Base shield intensity based on surface normal (mech's local forward = more visible)
    var baseIntensity = max(0.0, in.normal.z);
    var v = uDynamicUniforms.shieldPower;
    if (baseIntensity >= v)
    {
        baseIntensity = 0;
    }
    
    //baseIntensity = min(0.0, baseIntensity - uDynamicUniforms.shieldPower);
    
    return vec4f(finalColor, baseIntensity * baseIntensity);
}
