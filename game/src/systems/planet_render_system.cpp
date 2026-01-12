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
    CreateTextureBindGroupLayout();

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

    GetResourceSystem()->RequestResource("/textures/8081_earthmap4k.jpg", [this](ResourceSharedPtr pResource) {
        m_pEarthTexture = std::dynamic_pointer_cast<ResourceTexture2D>(pResource);
        m_TextureInitialized = true;
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

    view.each([this](const auto entity, PlanetComponent& planetComponent) {
        if (!planetComponent.initialized)
        {
            PlanetMeshGenerator::Generate(planetComponent, 24);
        }

        // Create texture bind group once the texture is loaded
        if (m_TextureInitialized && !planetComponent.textureBindGroup && m_TextureBindGroupLayout)
        {
            CreateTextureBindGroup(planetComponent);
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

            // Skip rendering if texture bind group is not ready
            if (!planetComponent.textureBindGroup)
            {
                return;
            }

            renderPass.SetPipeline(m_RenderPipeline);
            renderPass.SetBindGroup(1, planetComponent.textureBindGroup);
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

    // Pipeline layout with global uniforms and texture bind group
    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_TextureBindGroupLayout
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
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_N3_UV) },
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

void PlanetRenderSystem::CreateTextureBindGroupLayout()
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Create sampler for texture filtering
    wgpu::SamplerDescriptor samplerDesc{
        .label = "Planet texture sampler",
        .addressModeU = wgpu::AddressMode::Repeat,
        .addressModeV = wgpu::AddressMode::ClampToEdge,
        .addressModeW = wgpu::AddressMode::ClampToEdge,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear,
        .maxAnisotropy = 16
    };
    m_TextureSampler = device.CreateSampler(&samplerDesc);

    // Bind group layout for texture: sampler at 0, texture at 1
    std::array<wgpu::BindGroupLayoutEntry, 2> entries = { { { .binding = 0,
                                                                .visibility = wgpu::ShaderStage::Fragment,
                                                                .sampler = { .type = wgpu::SamplerBindingType::Filtering } },
        { .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture = {
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D } } } };

    wgpu::BindGroupLayoutDescriptor layoutDesc{
        .label = "Planet texture bind group layout",
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    m_TextureBindGroupLayout = device.CreateBindGroupLayout(&layoutDesc);
}

void PlanetRenderSystem::CreateTextureBindGroup(PlanetComponent& planetComponent)
{
    if (!m_pEarthTexture || !m_TextureBindGroupLayout)
    {
        return;
    }

    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Store reference to texture in the component
    planetComponent.colorTexture = m_pEarthTexture;

    std::array<wgpu::BindGroupEntry, 2> entries = { { { .binding = 0,
                                                          .sampler = m_TextureSampler },
        { .binding = 1,
            .textureView = m_pEarthTexture->GetTextureView() } } };

    wgpu::BindGroupDescriptor bindGroupDesc{
        .label = "Planet texture bind group",
        .layout = m_TextureBindGroupLayout,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    planetComponent.textureBindGroup = device.CreateBindGroup(&bindGroupDesc);
}

} // namespace WingsOfSteel
