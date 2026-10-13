#include "scene/systems/landscape_render_system.hpp"

#include <glm/glm.hpp>

#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/vertex_types.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "scene/components/landscape_component.hpp"
#include "scene/scene.hpp"

#include <array>
#include <vector>

namespace WingsOfSteel
{

LandscapeRenderSystem::LandscapeRenderSystem()
    : m_VertexCount(0)
    , m_Initialized(false)
{
    // The pipeline's layout needs the splat bind group layout, so these have to exist before the
    // shader has a chance to come back.
    CreateSplatResources();

    // Load the landscape shader
    GetResourceSystem()->RequestResource("/shaders/landscape.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
        HandleShaderInjection();
        m_Initialized = true;
    });
}

LandscapeRenderSystem::~LandscapeRenderSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void LandscapeRenderSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const LandscapeComponent>();

    view.each([this](const auto entity, const LandscapeComponent& landscapeComponent) {
        // The mesh only depends on the shape of the terrain, which changes rarely. The splat
        // weights change constantly - every plant on the island reshapes them as it grows - so the
        // two are deliberately kept apart: retinting the ground must not rebuild the geometry.
        if (landscapeComponent.Generation != this->m_Generation)
        {
            GenerateGeometry(landscapeComponent);
            this->m_Generation = landscapeComponent.Generation;
        }

        if (landscapeComponent.SplatWeightsGeneration != this->m_SplatWeightsGeneration)
        {
            UpdateSplatTexture(landscapeComponent);
            this->m_SplatWeightsGeneration = landscapeComponent.SplatWeightsGeneration;
        }

        // Small enough that tracking whether it changed would cost more than writing it, which is
        // what makes editing a splat color free.
        UpdateSplatUniforms(landscapeComponent);
    });
}

void LandscapeRenderSystem::GenerateGeometry(const LandscapeComponent& landscapeComponent)
{
    const uint32_t landscapeSize = landscapeComponent.Width;
    const float cellSize = landscapeComponent.CellSize;
    const float halfGridSize = (landscapeSize * cellSize) / 2.0f;

    const float maxHeight = landscapeComponent.Height;

    auto getHeight = [&landscapeComponent, maxHeight](int x, int z) -> float {
        x = glm::clamp(x, 0, (int)(landscapeComponent.Width - 1));
        z = glm::clamp(z, 0, (int)(landscapeComponent.Length - 1));
        return landscapeComponent.Heightmap[x + z * landscapeComponent.Width] * maxHeight - landscapeComponent.WaterLevel;
    };

    // Generate unique vertices: (landscapeSize+1) x (landscapeSize+1)
    // Neither normals nor colors are stored: the shader reconstructs the face normal from
    // screen-space derivatives of the world position, giving per-triangle flat shading, and takes
    // the color from the splat texture, sampled by that same world position.
    std::vector<glm::vec3> vertices;
    const uint32_t vertexGridSize = landscapeSize + 1;
    vertices.reserve(vertexGridSize * vertexGridSize);

    for (int z = 0; z < vertexGridSize; ++z)
    {
        for (int x = 0; x < vertexGridSize; ++x)
        {
            const float worldX = x * cellSize - halfGridSize;
            const float worldZ = z * cellSize - halfGridSize;
            const float height = getHeight(x, z);

            vertices.push_back(glm::vec3(worldX, height, worldZ));
        }
    }

    // Generate indices for triangles
    std::vector<uint32_t> indices;
    indices.reserve(landscapeSize * landscapeSize * 6); // 2 triangles per cell, 3 indices per triangle

    for (uint32_t z = 0; z < landscapeSize; ++z)
    {
        for (uint32_t x = 0; x < landscapeSize; ++x)
        {
            // Calculate vertex indices for this quad
            uint32_t topLeft = z * vertexGridSize + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * vertexGridSize + x;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle (counter-clockwise from above: bottomRight, topRight, topLeft)
            indices.push_back(bottomRight);
            indices.push_back(topRight);
            indices.push_back(topLeft);

            // Second triangle (counter-clockwise from above: bottomLeft, bottomRight, topLeft)
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
            indices.push_back(topLeft);
        }
    }

    m_VertexCount = static_cast<uint32_t>(vertices.size());
    m_IndexCount = static_cast<uint32_t>(indices.size());

    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Create vertex buffer
    if (m_VertexCount > 0)
    {
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Landscape vertex buffer",
            .usage = wgpu::BufferUsage::Vertex,
            .size = vertices.size() * sizeof(glm::vec3),
            .mappedAtCreation = true
        };
        m_VertexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(m_VertexBuffer.GetMappedRange(), vertices.data(), vertices.size() * sizeof(glm::vec3));
        m_VertexBuffer.Unmap();
    }

    // Create index buffer
    if (m_IndexCount > 0)
    {
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Landscape index buffer",
            .usage = wgpu::BufferUsage::Index,
            .size = indices.size() * sizeof(uint32_t),
            .mappedAtCreation = true
        };
        m_IndexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(m_IndexBuffer.GetMappedRange(), indices.data(), indices.size() * sizeof(uint32_t));
        m_IndexBuffer.Unmap();
    }
}

// The uniform buffer, the sampler and the bind group layout outlive any particular landscape, so
// they are made once. The texture starts out as a 1x1 placeholder rather than being left null: a
// bind group has to be complete before it can be built, and a landscape nothing has splatted yet
// has to render as bare terrain rather than not at all.
void LandscapeRenderSystem::CreateSplatResources()
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Landscape splat uniforms buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(SplatUniforms)
    };
    m_SplatUniformsBuffer = device.CreateBuffer(&bufferDescriptor);

    // Clamped rather than repeating, so the outermost ring of mesh vertices - which sits half a
    // cell past the last splat texel - reuses the edge weights instead of wrapping around to the
    // far side of the island.
    wgpu::SamplerDescriptor samplerDescriptor{
        .addressModeU = wgpu::AddressMode::ClampToEdge,
        .addressModeV = wgpu::AddressMode::ClampToEdge,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Nearest
    };
    m_SplatSampler = device.CreateSampler(&samplerDescriptor);

    std::array<wgpu::BindGroupLayoutEntry, 3> layoutEntries{
        wgpu::BindGroupLayoutEntry{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .buffer{
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(SplatUniforms) } },
        wgpu::BindGroupLayoutEntry{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2DArray } },
        wgpu::BindGroupLayoutEntry{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler{
                .type = wgpu::SamplerBindingType::Filtering } }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .label = "Landscape splat bind group layout",
        .entryCount = static_cast<uint32_t>(layoutEntries.size()),
        .entries = layoutEntries.data()
    };
    m_SplatBindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    CreateSplatTexture(1, 1);
}

// Also rebuilds the bind group, which holds the texture's view and so can't outlive it.
void LandscapeRenderSystem::CreateSplatTexture(uint32_t width, uint32_t length)
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    // No mip chain: the terrain is always seen at roughly the same scale, and a single level keeps
    // an update down to one WriteTexture with no render passes to generate the smaller levels.
    wgpu::TextureDescriptor textureDescriptor{
        .label = "Landscape splat weights",
        .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
        .dimension = wgpu::TextureDimension::e2D,
        .size = { width, length, sSplatPlaneCount },
        .format = wgpu::TextureFormat::RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1
    };
    m_SplatTexture = device.CreateTexture(&textureDescriptor);

    wgpu::TextureViewDescriptor textureViewDescriptor{
        .format = textureDescriptor.format,
        .dimension = wgpu::TextureViewDimension::e2DArray,
        .mipLevelCount = 1,
        .arrayLayerCount = sSplatPlaneCount
    };
    m_SplatTextureView = m_SplatTexture.CreateView(&textureViewDescriptor);

    std::array<wgpu::BindGroupEntry, 3> entries{
        wgpu::BindGroupEntry{
            .binding = 0,
            .buffer = m_SplatUniformsBuffer,
            .offset = 0,
            .size = sizeof(SplatUniforms) },
        wgpu::BindGroupEntry{
            .binding = 1,
            .textureView = m_SplatTextureView },
        wgpu::BindGroupEntry{
            .binding = 2,
            .sampler = m_SplatSampler }
    };

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .label = "Landscape splat bind group",
        .layout = m_SplatBindGroupLayout,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    m_SplatBindGroup = device.CreateBindGroup(&bindGroupDescriptor);

    m_SplatWidth = width;
    m_SplatLength = length;
}

void LandscapeRenderSystem::UpdateSplatTexture(const LandscapeComponent& landscapeComponent)
{
    const size_t expectedSize = static_cast<size_t>(landscapeComponent.Width) * landscapeComponent.Length * LandscapeComponent::kMaxSplatLayers;
    const bool hasWeights = (expectedSize > 0) && (landscapeComponent.SplatWeights.size() == expectedSize);

    // Nothing has splatted this landscape, so it falls back to a single texel of zero weights and
    // renders as bare terrain everywhere.
    static constexpr std::array<uint8_t, LandscapeComponent::kMaxSplatLayers> sEmptyWeights{};

    const uint32_t width = hasWeights ? landscapeComponent.Width : 1;
    const uint32_t length = hasWeights ? landscapeComponent.Length : 1;
    const uint8_t* pWeights = hasWeights ? landscapeComponent.SplatWeights.data() : sEmptyWeights.data();

    if (width != m_SplatWidth || length != m_SplatLength)
    {
        CreateSplatTexture(width, length);
    }

    wgpu::TexelCopyTextureInfo destination{
        .texture = m_SplatTexture,
        .mipLevel = 0,
        .origin = { 0, 0, 0 },
        .aspect = wgpu::TextureAspect::All
    };

    // The weights are stored plane-major, which is exactly how an array texture wants them: all of
    // plane 0's rows, then all of plane 1's. That lets every layer go up in a single call.
    wgpu::TexelCopyBufferLayout sourceLayout{
        .offset = 0,
        .bytesPerRow = width * 4,
        .rowsPerImage = length
    };

    wgpu::Extent3D writeSize{ width, length, sSplatPlaneCount };
    const size_t dataSize = static_cast<size_t>(width) * length * LandscapeComponent::kMaxSplatLayers;
    GetRenderSystem()->GetDevice().GetQueue().WriteTexture(&destination, pWeights, dataSize, &sourceLayout, &writeSize);
}

void LandscapeRenderSystem::UpdateSplatUniforms(const LandscapeComponent& landscapeComponent)
{
    SplatUniforms uniforms{};
    for (size_t i = 0; i < LandscapeComponent::kMaxSplatLayers; ++i)
    {
        uniforms.colors[i] = glm::vec4(landscapeComponent.SplatColors[i], 0.0f);
    }
    uniforms.baseColor = glm::vec4(landscapeComponent.SplatBaseColor, 0.0f);

    // The texture's dimensions rather than the landscape's: they differ while the placeholder is
    // bound, and it is the texture the shader has to find the center of a texel in.
    uniforms.params = glm::vec4(
        landscapeComponent.Width * landscapeComponent.CellSize,
        landscapeComponent.Length * landscapeComponent.CellSize,
        static_cast<float>(m_SplatWidth),
        static_cast<float>(m_SplatLength));

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_SplatUniformsBuffer, 0, &uniforms, sizeof(SplatUniforms));
}

void LandscapeRenderSystem::CreateRenderPipeline()
{
    if (!m_pShader)
    {
        return;
    }

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSceneFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    // Pipeline layout with the global uniforms and the splat map
    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_SplatBindGroupLayout
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
        .label = "Landscape render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::Back },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void LandscapeRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    // Draw the grid if the pipeline is ready and we have vertices and indices
    if (m_RenderPipeline && m_VertexBuffer && m_IndexBuffer && m_IndexCount > 0)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetBindGroup(1, m_SplatBindGroup);
        renderPass.SetVertexBuffer(0, m_VertexBuffer);
        renderPass.SetIndexBuffer(m_IndexBuffer, wgpu::IndexFormat::Uint32);
        renderPass.DrawIndexed(m_IndexCount);
    }
}

void LandscapeRenderSystem::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                CreateRenderPipeline();
            });
    }
}

} // namespace WingsOfSteel
