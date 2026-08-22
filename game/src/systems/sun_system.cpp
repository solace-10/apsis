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
        CreateRenderPipeline();
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

void SunSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (!m_RenderPipeline)
    {
        return;
    }

    renderPass.SetPipeline(m_RenderPipeline);
    renderPass.Draw(kSunVertexCount);
}

void SunSystem::CreateRenderPipeline()
{
    if (!m_pShader)
    {
        return;
    }

    // Premultiplied alpha, matching the atmosphere: the shader multiplies the Sun's colour by
    // its own coverage, so the colour goes on at full strength and the alpha only says how much
    // of what is behind survives.
    wgpu::BlendState blendState{
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat(),
        .blend = &blendState,
        .writeMask = wgpu::ColorWriteMask::All
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
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

    // Tested but not written. The Sun is drawn before anything else in the sector pass, so the
    // test never rejects anything today; keeping it as Less rather than Always means the Earth
    // still eclipses the disc if that order is ever changed.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Less
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Sun render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 0,
            .buffers = nullptr },
        // No culling: the quad is built from the camera's own axes, so which way it is wound
        // depends on where the camera has orbited to.
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };

    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void SunSystem::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (m_pShader.get() == pResourceShader)
                {
                    CreateRenderPipeline();
                }
            });
    }
}

} // namespace WingsOfSteel
