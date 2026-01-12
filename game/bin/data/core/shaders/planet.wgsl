struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldNormal: vec3f,
    @location(1) uv: vec2f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@group(1) @binding(0) var textureSampler: sampler;
@group(1) @binding(1) var colorTexture: texture_2d<f32>;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.worldNormal = in.normal;
    out.uv = in.uv;
    return out;
}

// Convert linear color to sRGB (gamma correction)
fn linearToSrgb(linear: vec3f) -> vec3f 
{
    let cutoff = linear < vec3f(0.0031308);
    let higher = vec3f(1.055) * pow(linear, vec3f(1.0/2.4)) - vec3f(0.055);
    let lower = linear * vec3f(12.92);
    return select(higher, lower, cutoff);
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    let baseColor = textureSample(colorTexture, textureSampler, in.uv).rgb;
    let N = normalize(in.worldNormal);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let diffuse = max(dot(N, L), 0.0);
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let ambient = uGlobalUniforms.ambientLightColor.rgb;
    let finalColor = baseColor * (ambient + lightColor * diffuse);
    // Apply gamma correction since swap chain is BGRA8Unorm (not sRGB)
    return vec4f(linearToSrgb(finalColor), 1.0);
}
