// PBR Shader - Metallic-Roughness Workflow (GLTF 2.0 Specification)
// Cook-Torrance BRDF with GGX distribution

const PI: f32 = 3.14159265359;

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
    @location(2) uv: vec2f
};

struct LocalUniforms
{
    modelMatrix: mat4x4<f32>
};

struct InstanceUniforms
{
    transform: array<mat4x4<f32>>
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;
@group(1) @binding(0) var<uniform> uLocalUniforms: LocalUniforms;
@group(2) @binding(0) var<storage, read> uInstanceUniforms: InstanceUniforms;
@group(3) @binding(0) var defaultSampler: sampler;
@group(3) @binding(1) var baseColorTexture: texture_2d<f32>;
@group(3) @binding(2) var metallicRoughnessTexture: texture_2d<f32>;
@group(3) @binding(3) var normalTexture: texture_2d<f32>;
@group(3) @binding(4) var occlusionTexture: texture_2d<f32>;
@group(3) @binding(5) var emissiveTexture: texture_2d<f32>;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    let modelMatrix = uInstanceUniforms.transform[in.instanceIdx] * uLocalUniforms.modelMatrix;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * modelMatrix * vec4f(in.position, 1.0);
    out.worldPosition = (modelMatrix * vec4f(in.position, 1.0)).xyz;
    out.worldNormal = normalize((modelMatrix * vec4f(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    return out;
}

// Compute TBN matrix from screen-space derivatives (for normal mapping without tangent attributes)
fn computeTBN(worldPos: vec3f, worldNormal: vec3f, uv: vec2f) -> mat3x3<f32>
{
    let dPdx = dpdx(worldPos);
    let dPdy = dpdy(worldPos);
    let dUVdx = dpdx(uv);
    let dUVdy = dpdy(uv);

    // Solve the linear system to get tangent and bitangent
    let r = 1.0 / (dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x);
    let tangent = normalize((dPdx * dUVdy.y - dPdy * dUVdx.y) * r);
    let bitangent = normalize((dPdy * dUVdx.x - dPdx * dUVdy.x) * r);

    // Re-orthogonalize tangent with respect to normal using Gram-Schmidt
    let N = normalize(worldNormal);
    let T = normalize(tangent - N * dot(N, tangent));
    let B = cross(N, T);

    return mat3x3<f32>(T, B, N);
}

// Fresnel-Schlick approximation
fn fresnelSchlick(cosTheta: f32, F0: vec3f) -> vec3f
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz Normal Distribution Function
fn distributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32
{
    let a = roughness * roughness;
    let a2 = a * a;
    let NdotH = max(dot(N, H), 0.0);
    let NdotH2 = NdotH * NdotH;

    let num = a2;
    let denom = (NdotH2 * (a2 - 1.0) + 1.0);
    let denomSq = PI * denom * denom;

    return num / denomSq;
}

// Schlick-GGX Geometry Function (single direction)
fn geometrySchlickGGX(NdotV: f32, roughness: f32) -> f32
{
    let r = roughness + 1.0;
    let k = (r * r) / 8.0;

    let num = NdotV;
    let denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// Smith's Geometry Function (both directions)
fn geometrySmith(N: vec3f, V: vec3f, L: vec3f, roughness: f32) -> f32
{
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let ggx1 = geometrySchlickGGX(NdotV, roughness);
    let ggx2 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    // All textures will already be in linear space: base color and emissive
    // are converted into linear when they're loaded.
    let baseColor = textureSample(baseColorTexture, defaultSampler, in.uv).rgb;

    let metallicRoughness = textureSample(metallicRoughnessTexture, defaultSampler, in.uv);
    let metallic = metallicRoughness.b; // GLTF: metallic in blue channel
    let roughness = metallicRoughness.g; // GLTF: roughness in green channel

    let normalMapSample = textureSample(normalTexture, defaultSampler, in.uv).rgb;
    let tangentNormal = normalMapSample * 2.0 - 1.0; // Unpack from [0,1] to [-1,1]

    var ao = textureSample(occlusionTexture, defaultSampler, in.uv).r;
    let emissive = textureSample(emissiveTexture, defaultSampler, in.uv).rgb;

    // Compute TBN matrix and transform normal to world space
    let TBN = computeTBN(in.worldPosition, in.worldNormal, in.uv);
    let N = normalize(TBN * tangentNormal);

    // View and light directions
    let V = normalize(uGlobalUniforms.cameraPosition.xyz - in.worldPosition);
    
    // Original: why is the light direction flipped?
    //let L = normalize(-uGlobalUniforms.directionalLightDirection.xyz);
    
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let H = normalize(V + L);

    // Calculate reflectance at normal incidence (F0)
    // Dielectrics use 0.04, metals use the base color
    let F0 = mix(vec3f(0.04), baseColor, metallic);

    // Cook-Torrance BRDF
    let NDF = distributionGGX(N, H, roughness);
    let G = geometrySmith(N, V, L, roughness);
    let F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Specular contribution
    let numerator = NDF * G * F;
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let denominator = 4.0 * NdotV * NdotL + 0.0001; // Prevent divide by zero
    let specular = numerator / denominator;

    // Energy conservation: diffuse and specular cannot exceed 1.0
    let kS = F; // Specular contribution (Fresnel term)
    let kD = (vec3f(1.0) - kS) * (1.0 - metallic); // Diffuse only for non-metals

    // Lambertian diffuse
    let diffuse = kD * baseColor / PI;

    // Outgoing radiance from directional light
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let Lo = (diffuse + specular) * lightColor * NdotL;

    // Ambient lighting with occlusion
    let ambient = uGlobalUniforms.ambientLightColor.rgb * baseColor * ao;

    // Final color: ambient + direct lighting + emissive
    let color = ambient + Lo + emissive;

	//return vec4f(metallic, metallic, metallic, 1.0);
	//return vec4f(roughness, roughness, roughness, 1.0);
	//return vec4f(baseColor, 1.0);
	//return vec4f(uGlobalUniforms.ambientLightColor.rgb, 1.0);
	//return vec4f(NdotL, NdotL, NdotL, 1.0);

    //return vec4f(color, baseColorSRGB.a);
    
    //return vec4f(ambient.rgb, 1.0);
    
    // Convert from linear to sRGB.
    // This should be done by the post-processing chain, not this shader!
    let output = pow(color.rgb, vec3f(1.0 / 2.2));
    return vec4f(output.rgb, 1.0);
}
