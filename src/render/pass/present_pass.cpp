#include "render/pass/present_pass.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "pandora.hpp"
#include "render/lighting/lighting_system.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"

namespace WingsOfSteel
{

static constexpr uint32_t kBloomLevelCount = 6;

// The shaders are embedded rather than loaded from /shaders, which would have every game ship its
// own copy.
static const char* sFullscreenTriangleShader = R"(
struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f
};

// One triangle large enough to cover the target.
@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput
{
    let xy = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    var out: VertexOutput;
    out.position = vec4f(xy * 2.0 - 1.0, 0.0, 1.0);
    out.uv = vec2f(xy.x, 1.0 - xy.y);
    return out;
}
)";

// The filters from Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare"
// (SIGGRAPH 2014).
static const char* sBloomShader = R"(
@group(0) @binding(0) var source: texture_2d<f32>;
@group(0) @binding(1) var sourceSampler: sampler;

fn tap(uv: vec2f, x: f32, y: f32) -> vec3f
{
    let texelSize = 1.0 / vec2f(textureDimensions(source));
    return textureSampleLevel(source, sourceSampler, uv + vec2f(x, y) * texelSize, 0.0).rgb;
}

@fragment fn downsampleMain(in: VertexOutput) -> @location(0) vec4f
{
    let a = tap(in.uv, -2.0, -2.0);
    let b = tap(in.uv, 0.0, -2.0);
    let c = tap(in.uv, 2.0, -2.0);
    let d = tap(in.uv, -2.0, 0.0);
    let e = tap(in.uv, 0.0, 0.0);
    let f = tap(in.uv, 2.0, 0.0);
    let g = tap(in.uv, -2.0, 2.0);
    let h = tap(in.uv, 0.0, 2.0);
    let i = tap(in.uv, 2.0, 2.0);
    let j = tap(in.uv, -1.0, -1.0);
    let k = tap(in.uv, 1.0, -1.0);
    let l = tap(in.uv, -1.0, 1.0);
    let m = tap(in.uv, 1.0, 1.0);

    // Five overlapping 2x2 boxes: the centre one carries half the weight, the four corner ones the rest.
    let color = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    return vec4f(color, 1.0);
}

// A 3x3 tent over the smaller level. The blend state adds it onto the larger one.
@fragment fn upsampleMain(in: VertexOutput) -> @location(0) vec4f
{
    let corners = tap(in.uv, -1.0, -1.0) + tap(in.uv, 1.0, -1.0) + tap(in.uv, -1.0, 1.0) + tap(in.uv, 1.0, 1.0);
    let edges = tap(in.uv, 0.0, -1.0) + tap(in.uv, -1.0, 0.0) + tap(in.uv, 1.0, 0.0) + tap(in.uv, 0.0, 1.0);
    return vec4f((corners + edges * 2.0 + tap(in.uv, 0.0, 0.0) * 4.0) / 16.0, 1.0);
}
)";

static const char* sPresentShader = R"(
struct Settings
{
    exposure: f32,
    bloomStrength: f32,
    // The upsample adds every level together; this turns the sum back into an average.
    bloomScale: f32,
    // A Tonemapper.
    tonemapper: u32
};

@group(0) @binding(0) var sceneTexture: texture_2d<f32>;
@group(0) @binding(1) var<uniform> settings: Settings;
@group(0) @binding(2) var overlayTexture: texture_2d<f32>;
@group(0) @binding(3) var bloomTexture: texture_2d<f32>;
@group(0) @binding(4) var bloomSampler: sampler;

// Khronos PBR Neutral, without its toe. The toe subtracts up to 0.04 from every channel, which suits
// a lit product but would roughly halve the brightness of a day side that sits around 0.05.
// What is left is the identity below kStartCompression and a shoulder that rolls highlights off
// towards white instead of clipping them.
const kStartCompression: f32 = 0.76;
const kDesaturation: f32 = 0.15;

fn tonemapNeutral(color: vec3f) -> vec3f
{
    let peak = max(color.r, max(color.g, color.b));
    if (peak < kStartCompression)
    {
        return color;
    }

    let d = 1.0 - kStartCompression;
    let newPeak = 1.0 - d * d / (peak + d - kStartCompression);
    let g = 1.0 - 1.0 / (kDesaturation * (peak - newPeak) + 1.0);
    return mix(color * (newPeak / peak), vec3f(newPeak), g);
}

// Hill's fit of the ACES RRT and sRGB ODT. The matrices are written as rows, hence color * m.
const kAcesInput = mat3x3f(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777);
const kAcesOutput = mat3x3f(
    1.60475, -0.53108, -0.07367,
    -0.10208, 1.10813, -0.00605,
    -0.00327, -0.07276, 1.07602);

fn tonemapAces(color: vec3f) -> vec3f
{
    let v = color * kAcesInput;
    let a = v * (v + 0.0245786) - 0.000090537;
    let b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return saturate((a / b) * kAcesOutput);
}

// Wrensch's minimal AgX, with the default look: https://iolite-engine.com/blog_posts/minimal_agx_implementation
const kAgxInset = mat3x3f(
    0.842479062253094, 0.0423282422610123, 0.0423756549057051,
    0.0784335999999992, 0.878468636469772, 0.0784336,
    0.0792237451477643, 0.0791661274605434, 0.879142973793104);
const kAgxOutset = mat3x3f(
    1.19687900512017, -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
const kAgxMinEv: f32 = -12.47393;
const kAgxMaxEv: f32 = 4.026069;

fn tonemapAgx(color: vec3f) -> vec3f
{
    let encoded = (clamp(log2(max(kAgxInset * color, vec3f(1e-10))), vec3f(kAgxMinEv), vec3f(kAgxMaxEv)) - kAgxMinEv) / (kAgxMaxEv - kAgxMinEv);
    let x2 = encoded * encoded;
    let x4 = x2 * x2;
    let curve = 15.5 * x4 * x2 - 40.14 * x4 * encoded + 31.96 * x4 - 6.868 * x2 * encoded + 0.4298 * x2 + 0.1191 * encoded - 0.00232;
    // The curve's output is display-encoded with a 2.2 gamma; decode it, as linearToSrgb follows.
    return pow(max(kAgxOutset * curve, vec3f(0.0)), vec3f(2.2));
}

fn tonemap(color: vec3f) -> vec3f
{
    switch settings.tonemapper
    {
        case 1u: { return tonemapAces(color); }
        case 2u: { return tonemapAgx(color); }
        default: { return tonemapNeutral(color); }
    }
}

fn linearToSrgb(linear: vec3f) -> vec3f
{
    let cutoff = linear < vec3f(0.0031308);
    let higher = vec3f(1.055) * pow(linear, vec3f(1.0 / 2.4)) - vec3f(0.055);
    let lower = linear * vec3f(12.92);
    return select(higher, lower, cutoff);
}

@fragment fn presentMain(in: VertexOutput) -> @location(0) vec4f
{
    // The scene and the overlay are the surface's size, so they are read texel for texel.
    let texel = vec2u(in.position.xy);
    let scene = textureLoad(sceneTexture, texel, 0).rgb;
    let bloom = textureSampleLevel(bloomTexture, bloomSampler, in.uv, 0.0).rgb * settings.bloomScale;
    let color = linearToSrgb(tonemap(mix(scene, bloom, settings.bloomStrength) * settings.exposure));
    let overlay = textureLoad(overlayTexture, texel, 0);
    return vec4f(color * (1.0 - overlay.a) + overlay.rgb, 1.0);
}
)";

PresentPass::PresentPass()
    : Pass("Present pass")
{
}

void PresentPass::Execute(wgpu::CommandEncoder& encoder)
{
    // Created here rather than in the constructor, as the engine's default passes are
    // constructed before the window, whose surface format the pipeline needs.
    if (!m_Pipeline)
    {
        CreateRenderPipelines();
    }

    UpdateBloomTexture();

    const LightingSystem* pLightingSystem = GetRenderSystem()->GetLightingSystem();
    const float bloomStrength = pLightingSystem->GetBloomStrength();
    if (bloomStrength > 0.0f)
    {
        RenderBloom(encoder);
    }

    const struct
    {
        float exposure;
        float bloomStrength;
        float bloomScale;
        Tonemapper tonemapper;
    } settings{
        std::exp2(pLightingSystem->GetExposure()),
        bloomStrength,
        1.0f / static_cast<float>(m_BloomLevelViews.size()),
        pLightingSystem->GetTonemapper()
    };
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_SettingsBuffer, 0, &settings, sizeof(settings));

    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    // Rebuilt every frame, as resizing the window replaces the textures.
    std::array<wgpu::BindGroupEntry, 5> bindGroupEntries{ {
        { .binding = 0, .textureView = GetWindow()->GetSceneColorTexture().GetTextureView() },
        { .binding = 1, .buffer = m_SettingsBuffer, .size = sizeof(settings) },
        { .binding = 2, .textureView = GetWindow()->GetOverlayColorTexture().GetTextureView() },
        { .binding = 3, .textureView = m_BloomLevelViews[0] },
        { .binding = 4, .sampler = m_Sampler },
    } };

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_Pipeline.GetBindGroupLayout(0),
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data()
    };
    wgpu::BindGroup bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }
    };

    wgpu::RenderPassDescriptor renderPassDescriptor{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
    renderPass.SetPipeline(m_Pipeline);
    renderPass.SetBindGroup(0, bindGroup);
    renderPass.Draw(3);
    renderPass.End();
}

void PresentPass::CreateRenderPipelines()
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    auto createModule = [&device](const char* pLabel, const char* pFragmentCode) {
        const std::string code = std::string(sFullscreenTriangleShader) + pFragmentCode;
        wgpu::ShaderSourceWGSL wgslDesc{};
        wgslDesc.code = code.c_str();

        wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
            .nextInChain = &wgslDesc,
            .label = pLabel
        };
        return device.CreateShaderModule(&shaderModuleDescriptor);
    };

    auto createPipeline = [&device](const char* pLabel, const wgpu::ShaderModule& module, const char* pEntryPoint, wgpu::TextureFormat format, const wgpu::BlendState* pBlendState = nullptr) {
        wgpu::ColorTargetState colorTargetState{
            .format = format,
            .blend = pBlendState
        };

        wgpu::FragmentState fragmentState{
            .module = module,
            .entryPoint = pEntryPoint,
            .targetCount = 1,
            .targets = &colorTargetState
        };

        wgpu::RenderPipelineDescriptor descriptor{
            .label = pLabel,
            .vertex = {
                .module = module,
                .entryPoint = "vertexMain" },
            .fragment = &fragmentState
        };
        return device.CreateRenderPipeline(&descriptor);
    };

    const wgpu::ShaderModule presentModule = createModule("Present shader", sPresentShader);
    m_Pipeline = createPipeline("Present pipeline", presentModule, "presentMain", GetWindow()->GetSurfaceFormat());

    const wgpu::ShaderModule bloomModule = createModule("Bloom shader", sBloomShader);
    const wgpu::TextureFormat bloomFormat = GetWindow()->GetSceneFormat();
    const wgpu::BlendState additive{
        .color = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::One },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::One }
    };
    m_DownsamplePipeline = createPipeline("Bloom downsample pipeline", bloomModule, "downsampleMain", bloomFormat);
    m_UpsamplePipeline = createPipeline("Bloom upsample pipeline", bloomModule, "upsampleMain", bloomFormat, &additive);

    wgpu::SamplerDescriptor samplerDescriptor{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear
    };
    m_Sampler = device.CreateSampler(&samplerDescriptor);

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Present settings buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = 4 * sizeof(float)
    };
    m_SettingsBuffer = device.CreateBuffer(&bufferDescriptor);
}

void PresentPass::UpdateBloomTexture()
{
    const uint32_t width = std::max(1u, GetWindow()->GetWidth() / 2);
    const uint32_t height = std::max(1u, GetWindow()->GetHeight() / 2);
    if (m_BloomTexture && m_BloomTexture.GetWidth() == width && m_BloomTexture.GetHeight() == height)
    {
        return;
    }

    const uint32_t levelCount = std::min(kBloomLevelCount, static_cast<uint32_t>(std::log2(std::max(width, height))) + 1);
    wgpu::TextureDescriptor textureDescriptor{
        .label = "Bloom texture",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .size = { .width = width, .height = height },
        .format = GetWindow()->GetSceneFormat(),
        .mipLevelCount = levelCount
    };
    m_BloomTexture = GetRenderSystem()->GetDevice().CreateTexture(&textureDescriptor);

    m_BloomLevelViews.clear();
    for (uint32_t level = 0; level < levelCount; level++)
    {
        wgpu::TextureViewDescriptor viewDescriptor{
            .baseMipLevel = level,
            .mipLevelCount = 1
        };
        m_BloomLevelViews.push_back(m_BloomTexture.CreateView(&viewDescriptor));
    }
}

void PresentPass::RenderBloom(wgpu::CommandEncoder& encoder)
{
    const size_t levelCount = m_BloomLevelViews.size();

    DrawFullscreen(encoder, m_DownsamplePipeline, m_BloomLevelViews[0], wgpu::LoadOp::Clear, GetWindow()->GetSceneColorTexture().GetTextureView());
    for (size_t level = 1; level < levelCount; level++)
    {
        DrawFullscreen(encoder, m_DownsamplePipeline, m_BloomLevelViews[level], wgpu::LoadOp::Clear, m_BloomLevelViews[level - 1]);
    }

    for (size_t level = levelCount - 1; level > 0; level--)
    {
        DrawFullscreen(encoder, m_UpsamplePipeline, m_BloomLevelViews[level - 1], wgpu::LoadOp::Load, m_BloomLevelViews[level]);
    }
}

void PresentPass::DrawFullscreen(wgpu::CommandEncoder& encoder, const wgpu::RenderPipeline& pipeline, const wgpu::TextureView& target, wgpu::LoadOp loadOp, const wgpu::TextureView& source)
{
    std::array<wgpu::BindGroupEntry, 2> bindGroupEntries{ {
        { .binding = 0, .textureView = source },
        { .binding = 1, .sampler = m_Sampler },
    } };

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = pipeline.GetBindGroupLayout(0),
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data()
    };
    wgpu::BindGroup bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = target,
        .loadOp = loadOp,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 0.0 }
    };

    wgpu::RenderPassDescriptor renderPassDescriptor{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
    renderPass.SetPipeline(pipeline);
    renderPass.SetBindGroup(0, bindGroup);
    renderPass.Draw(3);
    renderPass.End();
}

} // namespace WingsOfSteel
