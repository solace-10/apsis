struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) color: vec3f,
    @location(2) normal: vec3f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) worldNormal: vec3f,
    @location(1) color: vec3f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.worldNormal = in.normal;
    out.color = in.color;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    let N = normalize(in.worldNormal);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let diffuse = max(dot(N, L), 0.0);
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let ambient = uGlobalUniforms.ambientLightColor.rgb;
    let finalColor = in.color * (ambient + lightColor * diffuse);
    return vec4f(finalColor, 1.0);
}
