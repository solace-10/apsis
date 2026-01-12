struct VertexInput
{
    @location(0) position: vec3f,
    @location(1) barycentric: vec3f
};

struct VertexOutput 
{
    @builtin(position) position: vec4f,
    @location(0) barycentric: vec3f
};

@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.barycentric = in.barycentric;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f 
{
    // Calculate distance to nearest edge using barycentric coordinates
    let bary = in.barycentric;
    let edgeFactor = min(min(bary.x, bary.y), bary.z);
    
    // Use screen-space derivatives for consistent line width
    let d = fwidth(edgeFactor);
    let lineWidth = 0.75;
    let edge = smoothstep(0.0, d * lineWidth, edgeFactor);
    
    // Discard interior pixels (keep only edges)
    if (edge > 0.98) {
        discard;
    }
    
    // Wireframe color (white)
    return vec4f(1.0, 1.0, 1.0, 1.0);
}
