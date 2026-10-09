#include "systems/sun_system.hpp"

#include <array>
#include <chrono>

#include <glm/vec3.hpp>
#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/directional_light_component.hpp>
#include <scene/scene.hpp>

#include "space/earth_frame.hpp"

namespace WingsOfSteel
{

// The quad is six vertices generated from the vertex index, so there is no vertex buffer and
// nothing per-instance to bind - see sun.wgsl.
static constexpr uint32_t kSunVertexCount = 6;

SunSystem::SunSystem()
{
    GetResourceSystem()->RequestResource("/shaders/sun.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipelines();
        HandleShaderInjection();
    });
}

SunSystem::~SunSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void SunSystem::Initialize(Scene* pScene)
{
}

void SunSystem::Update(float delta)
{
    if (!m_TrackRealSun || GetActiveScene() == nullptr)
    {
        return;
    }

    // World space is inertial: ECIToWorld() is a relabelling of the axes with no rotation in it,
    // and it is the planet's mesh that turns rather than the frame. So GMST plays no part here -
    // the Sun's direction in world space is its direction in ECI, read off in a different order.
    const glm::vec3 sunDirection(ECIToWorld(CalculateSunDirectionECI(std::chrono::system_clock::now())));

    auto view = GetActiveScene()->GetRegistry().view<DirectionalLightComponent>();
    view.each([&sunDirection](DirectionalLightComponent& directionalLightComponent) {
        directionalLightComponent.SetDirection(sunDirection);
    });
}

void SunSystem::RenderDisc(wgpu::RenderPassEncoder& renderPass)
{
    if (m_DiscPipeline)
    {
        renderPass.SetPipeline(m_DiscPipeline);
        renderPass.Draw(kSunVertexCount);
    }
}

void SunSystem::RenderGlare(wgpu::RenderPassEncoder& renderPass)
{
    if (m_GlarePipeline)
    {
        renderPass.SetPipeline(m_GlarePipeline);
        renderPass.Draw(kSunVertexCount);
    }
}

void SunSystem::CreateRenderPipelines()
{
    // The disc is depth tested and the glare is not; everything else about the two is identical,
    // down to sharing a vertex stage and a quad. sun.wgsl carries the reasoning.
    m_DiscPipeline = CreateRenderPipeline("Sun disc render pipeline", "discMain", wgpu::CompareFunction::Less);
    m_GlarePipeline = CreateRenderPipeline("Sun glare render pipeline", "aureoleMain", wgpu::CompareFunction::Always);
}

wgpu::RenderPipeline SunSystem::CreateRenderPipeline(const char* pLabel, const char* pFragmentEntryPoint, wgpu::CompareFunction depthCompare)
{
    if (!m_pShader)
    {
        return nullptr;
    }

    // Additive, not the premultiplied alpha the atmosphere uses. Both give the same result over
    // the black of space, but the glare is drawn over the planet's lit limb as well, and a light
    // source must never darken what is behind it - which any blend that scales the destination
    // will do wherever the destination is the brighter of the two.
    wgpu::BlendState blendState{
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::One },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::One }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSceneFormat(),
        .blend = &blendState,
        .writeMask = wgpu::ColorWriteMask::All
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .entryPoint = pFragmentEntryPoint,
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Nothing beyond the global uniforms: the shader takes the sun direction, the sun colour and
    // the camera from them.
    std::array<wgpu::BindGroupLayout, 1> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Never written to, by either pass. Always is how a pipeline in a pass that has a depth
    // attachment says it does not want the test - the attachment's format still has to be
    // declared or the pipeline is not compatible with the pass.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = depthCompare
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = pLabel,
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .entryPoint = "vertexMain",
            .bufferCount = 0,
            .buffers = nullptr },
        // No culling: the quad is built from the camera's own axes, so which way it is wound
        // depends on where the camera has orbited to.
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };

    return GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void SunSystem::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (m_pShader.get() == pResourceShader)
                {
                    CreateRenderPipelines();
                }
            });
    }
}

} // namespace WingsOfSteel
