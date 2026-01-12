#include "planet_render_system.hpp"

#include <array>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>

#include "components/planet_component.hpp"
#include "sector/planet_mesh_generator.hpp"

namespace WingsOfSteel
{

PlanetRenderSystem::PlanetRenderSystem()
{
    GetResourceSystem()->RequestResource("/shaders/planet.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
        HandleShaderInjection();
        m_Initialized = true;
    });

    GetResourceSystem()->RequestResource("/shaders/planet_wireframe.wgsl", [this](ResourceSharedPtr pResource) {
        m_pWireframeShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateWireframePipeline();
        m_WireframeInitialized = true;
    });
}

PlanetRenderSystem::~PlanetRenderSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void PlanetRenderSystem::Initialize(Scene* pScene)
{
}

void PlanetRenderSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<PlanetComponent>();

    view.each([](const auto entity, PlanetComponent& planetComponent) {
        if (!planetComponent.initialized)
        {
            PlanetMeshGenerator::Generate(planetComponent, 24);
        }
    });
}

void PlanetRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<PlanetComponent>();

    // Render solid mesh
    if (m_Initialized && m_RenderPipeline)
    {
        view.each([this, &renderPass](const auto entity, PlanetComponent& planetComponent) {
            if (!planetComponent.initialized || !planetComponent.vertexBuffer || !planetComponent.indexBuffer)
            {
                return;
            }

            renderPass.SetPipeline(m_RenderPipeline);
            renderPass.SetVertexBuffer(0, planetComponent.vertexBuffer);
            renderPass.SetIndexBuffer(planetComponent.indexBuffer, wgpu::IndexFormat::Uint32);
            renderPass.DrawIndexed(planetComponent.indexCount);
        });
    }

    // Render wireframe overlay
    if (m_WireframeInitialized && m_WireframePipeline && m_RenderWireframe)
    {
        view.each([this, &renderPass](const auto entity, PlanetComponent& planetComponent) {
            if (!planetComponent.initialized || !planetComponent.wireframeVertexBuffer)
            {
                return;
            }

            renderPass.SetPipeline(m_WireframePipeline);
            renderPass.SetVertexBuffer(0, planetComponent.wireframeVertexBuffer);
            renderPass.Draw(planetComponent.wireframeVertexCount);
        });
    }
}

void PlanetRenderSystem::CreateRenderPipeline()
{
    if (!m_pShader)
    {
        return;
    }

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with global uniforms only
    std::array<wgpu::BindGroupLayout, 1> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Depth state
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less
    };

    // Create the render pipeline
    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Planet render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_C3_N3) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::Back },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void PlanetRenderSystem::CreateWireframePipeline()
{
    if (!m_pWireframeShader)
    {
        return;
    }

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pWireframeShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with global uniforms only
    std::array<wgpu::BindGroupLayout, 1> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Depth state with bias to prevent z-fighting with the solid mesh
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::LessEqual,
        .depthBias = -1,
        .depthBiasSlopeScale = -1.0f
    };

    // Create the wireframe render pipeline
    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Planet wireframe render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pWireframeShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_B3) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::Back },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_WireframePipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void PlanetRenderSystem::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (m_pShader.get() == pResourceShader)
                {
                    CreateRenderPipeline();
                }
                else if (m_pWireframeShader.get() == pResourceShader)
                {
                    CreateWireframePipeline();
                }
            });
    }
}

} // namespace WingsOfSteel
