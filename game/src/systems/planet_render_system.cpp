#include "planet_render_system.hpp"

#include <array>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>

#include "components/atmosphere_component.hpp"
#include "components/planet_component.hpp"
#include "sector/planet_mesh_generator.hpp"

namespace WingsOfSteel
{

// Must match AtmosphereUniforms in atmosphere.wgsl
struct AtmosphereUniformData
{
    glm::vec3 color; // offset 0
    float height; // offset 12
    float density; // offset 16
    float _padding0; // offset 20
    float _padding1; // offset 24
    float _padding2; // offset 28
    // Total: 32 bytes
};

PlanetRenderSystem::PlanetRenderSystem()
{
    CreateTextureBindGroupLayout();
    CreateAtmosphereBindGroupLayout();

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

    GetResourceSystem()->RequestResource("/shaders/atmosphere.wgsl", [this](ResourceSharedPtr pResource) {
        m_pAtmosphereShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateAtmospherePipeline();
        m_AtmosphereInitialized = true;
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

    // Initialize planet components
    {
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

    // Initialize atmosphere components
    {
        auto view = registry.view<AtmosphereComponent>();
        view.each([this](const auto entity, AtmosphereComponent& atmosphereComponent) {
            if (!atmosphereComponent.initialized && m_AtmosphereBindGroupLayout)
            {
                InitializeAtmosphereComponent(atmosphereComponent);
            }
        });
    }
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

    // Render atmosphere (after planet, with alpha blending)
    if (m_AtmosphereInitialized && m_AtmospherePipeline)
    {
        auto atmosphereView = registry.view<PlanetComponent, AtmosphereComponent>();
        atmosphereView.each([this, &renderPass](const auto entity, PlanetComponent& planetComponent, AtmosphereComponent& atmosphereComponent) {
            if (!planetComponent.initialized || !atmosphereComponent.initialized)
            {
                return;
            }

            if (!planetComponent.vertexBuffer || !planetComponent.indexBuffer || !atmosphereComponent.bindGroup)
            {
                return;
            }

            // Update atmosphere uniforms in case they changed
            UpdateAtmosphereUniforms(atmosphereComponent);

            renderPass.SetPipeline(m_AtmospherePipeline);
            renderPass.SetBindGroup(1, atmosphereComponent.bindGroup);
            renderPass.SetVertexBuffer(0, planetComponent.vertexBuffer);
            renderPass.SetIndexBuffer(planetComponent.indexBuffer, wgpu::IndexFormat::Uint32);
            renderPass.DrawIndexed(planetComponent.indexCount);
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
                else if (m_pAtmosphereShader.get() == pResourceShader)
                {
                    CreateAtmospherePipeline();
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

void PlanetRenderSystem::CreateAtmospherePipeline()
{
    if (!m_pAtmosphereShader)
    {
        return;
    }

    // Alpha blending for transparent atmosphere
    wgpu::BlendState blendState{
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat(),
        .blend = &blendState,
        .writeMask = wgpu::ColorWriteMask::All
    };

    wgpu::FragmentState fragmentState{
        .module = m_pAtmosphereShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with global uniforms and atmosphere uniforms
    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_AtmosphereBindGroupLayout
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Depth state: test against depth but don't write (transparent)
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Less
    };

    // Create the atmosphere render pipeline
    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Atmosphere render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pAtmosphereShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_N3_UV) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::Back },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_AtmospherePipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void PlanetRenderSystem::CreateAtmosphereBindGroupLayout()
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Bind group layout for atmosphere uniforms
    wgpu::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = { .type = wgpu::BufferBindingType::Uniform }
    };

    wgpu::BindGroupLayoutDescriptor layoutDesc{
        .label = "Atmosphere uniform bind group layout",
        .entryCount = 1,
        .entries = &entry
    };
    m_AtmosphereBindGroupLayout = device.CreateBindGroupLayout(&layoutDesc);
}

void PlanetRenderSystem::InitializeAtmosphereComponent(AtmosphereComponent& atmosphereComponent)
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Create uniform buffer
    wgpu::BufferDescriptor bufferDesc{
        .label = "Atmosphere uniform buffer",
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
        .size = sizeof(AtmosphereUniformData)
    };
    atmosphereComponent.uniformBuffer = device.CreateBuffer(&bufferDesc);

    // Create bind group
    wgpu::BindGroupEntry entry{
        .binding = 0,
        .buffer = atmosphereComponent.uniformBuffer,
        .size = sizeof(AtmosphereUniformData)
    };

    wgpu::BindGroupDescriptor bindGroupDesc{
        .label = "Atmosphere uniform bind group",
        .layout = m_AtmosphereBindGroupLayout,
        .entryCount = 1,
        .entries = &entry
    };
    atmosphereComponent.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    // Write initial uniform data
    UpdateAtmosphereUniforms(atmosphereComponent);

    atmosphereComponent.initialized = true;
}

void PlanetRenderSystem::UpdateAtmosphereUniforms(AtmosphereComponent& atmosphereComponent)
{
    AtmosphereUniformData data{
        .color = atmosphereComponent.color,
        .height = atmosphereComponent.height,
        .density = atmosphereComponent.density,
        ._padding0 = 0.0f,
        ._padding1 = 0.0f,
        ._padding2 = 0.0f
    };

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(
        atmosphereComponent.uniformBuffer,
        0,
        &data,
        sizeof(AtmosphereUniformData));
}

} // namespace WingsOfSteel
