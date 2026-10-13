#include "scene/systems/sea_render_system.hpp"

#include <FastNoiseLite.h>
#include <glm/glm.hpp>

#include "imgui.h"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "render/vertex_types.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "scene/components/landscape_component.hpp"
#include "scene/components/sea_component.hpp"
#include "scene/scene.hpp"

#include <array>
#include <vector>

namespace WingsOfSteel
{

namespace
{

// Samples per landscape cell, along each axis. The landscape's cells are far too coarse to resolve
// a shoreline that foam a few meters wide is meant to hug, so the distance field is built on a
// finer grid interpolated out of the heightmap.
constexpr uint32_t ShoreFieldSupersample = 4;

// One cell of the 8SSEDT sweep: the offset, in cells, to the nearest land sample found so far.
struct ShoreSweepCell
{
    glm::vec2 Offset{ 0.0f, 0.0f };
    float DistanceSquared{ 0.0f };
};

// Pulls the neighbour's nearest land sample across, if reaching it through the neighbour is closer
// than anything this cell has seen. Running this over the eight neighbours in two sweeps is enough
// for every cell to end up with (very nearly) the nearest land sample in the whole grid.
void CompareShoreSweepCell(std::vector<ShoreSweepCell>& grid, ShoreSweepCell& cell, int x, int z, int offsetX, int offsetZ, int width, int length)
{
    const int neighbourX = x + offsetX;
    const int neighbourZ = z + offsetZ;
    if (neighbourX < 0 || neighbourX >= width || neighbourZ < 0 || neighbourZ >= length)
    {
        return;
    }

    const glm::vec2 offset = grid[neighbourX + neighbourZ * width].Offset + glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetZ));
    const float distanceSquared = glm::dot(offset, offset);
    if (distanceSquared < cell.DistanceSquared)
    {
        cell.Offset = offset;
        cell.DistanceSquared = distanceSquared;
    }
}

} // namespace

SeaRenderSystem::SeaRenderSystem()
{
    CreateUniforms();

    GetResourceSystem()->RequestResource("/shaders/sea.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();
        HandleShaderInjection();
        m_Initialized = true;
    });
}

SeaRenderSystem::~SeaRenderSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void SeaRenderSystem::Update(float delta)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();

    const LandscapeComponent* pLandscapeComponent = nullptr;
    auto landscapeView = registry.view<const LandscapeComponent>();
    for (const auto entity : landscapeView)
    {
        pLandscapeComponent = &landscapeView.get<const LandscapeComponent>(entity);
        break;
    }
    const uint32_t landscapeGeneration = pLandscapeComponent ? pLandscapeComponent->Generation : 0;

    auto view = registry.view<const SeaComponent>();
    view.each([this, pLandscapeComponent, landscapeGeneration](const auto entity, const SeaComponent& seaComponent) {
        // The sea also rebuilds when the landscape regenerates, as the baked water depths
        // follow the shape of the terrain.
        if (seaComponent.Generation != m_Generation || landscapeGeneration != m_LandscapeGeneration)
        {
            GenerateGeometry(seaComponent, pLandscapeComponent);
            m_Generation = seaComponent.Generation;
            m_LandscapeGeneration = landscapeGeneration;
        }

        UpdateUniforms(seaComponent);
    });
}

void SeaRenderSystem::GenerateGeometry(const SeaComponent& seaComponent, const LandscapeComponent* pLandscapeComponent)
{
    const float halfWidth = seaComponent.Width / 2.0f;
    const float halfLength = seaComponent.Length / 2.0f;
    const uint32_t cellsX = glm::max(1u, static_cast<uint32_t>(glm::round(seaComponent.Width / seaComponent.CellSize)));
    const uint32_t cellsZ = glm::max(1u, static_cast<uint32_t>(glm::round(seaComponent.Length / seaComponent.CellSize)));
    const float stepX = seaComponent.Width / cellsX;
    const float stepZ = seaComponent.Length / cellsZ;

    // How deep the water is under a given point, bilinearly sampled from the landscape's
    // heightmap. The terrain is rendered at (height * Height - WaterLevel), so the water
    // surface always sits at world y = 0 and depth is just the negated terrain height.
    auto getWaterDepth = [pLandscapeComponent](float worldX, float worldZ) -> float {
        constexpr float OpenSeaDepth = 1000.0f;
        if (!pLandscapeComponent || pLandscapeComponent->Heightmap.empty())
        {
            return OpenSeaDepth;
        }

        const LandscapeComponent& landscape = *pLandscapeComponent;
        auto getTerrainHeight = [&landscape](int x, int z) -> float {
            x = glm::clamp(x, 0, static_cast<int>(landscape.Width) - 1);
            z = glm::clamp(z, 0, static_cast<int>(landscape.Length) - 1);
            return landscape.Heightmap[x + z * landscape.Width] * landscape.Height - landscape.WaterLevel;
        };

        const float fx = (worldX + (landscape.Width * landscape.CellSize) / 2.0f) / landscape.CellSize;
        const float fz = (worldZ + (landscape.Length * landscape.CellSize) / 2.0f) / landscape.CellSize;
        const int x0 = static_cast<int>(glm::floor(fx));
        const int z0 = static_cast<int>(glm::floor(fz));
        const float tx = fx - x0;
        const float tz = fz - z0;

        const float height = glm::mix(
            glm::mix(getTerrainHeight(x0, z0), getTerrainHeight(x0 + 1, z0), tx),
            glm::mix(getTerrainHeight(x0, z0 + 1), getTerrainHeight(x0 + 1, z0 + 1), tx),
            tz);
        return glm::max(0.0f, -height);
    };

    const ShoreDistanceField shoreDistanceField = BuildShoreDistanceField(pLandscapeComponent);

    // The vertex color carries the shape of the coastline rather than an actual color, as the sea
    // is shaded procedurally: x is the water depth, y the distance to the shore and z the phase
    // the shore waves arrive on.
    std::vector<VertexP3C3> vertices;
    const uint32_t vertexGridX = cellsX + 1;
    const uint32_t vertexGridZ = cellsZ + 1;
    vertices.reserve(vertexGridX * vertexGridZ);

    for (uint32_t z = 0; z < vertexGridZ; ++z)
    {
        for (uint32_t x = 0; x < vertexGridX; ++x)
        {
            const float worldX = x * stepX - halfWidth;
            const float worldZ = z * stepZ - halfLength;
            const glm::vec3 shoreData(
                getWaterDepth(worldX, worldZ),
                shoreDistanceField.Sample(shoreDistanceField.Distance, worldX, worldZ),
                shoreDistanceField.Sample(shoreDistanceField.Phase, worldX, worldZ));
            vertices.push_back({ glm::vec3(worldX, 0.0f, worldZ), shoreData });
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(cellsX * cellsZ * 6);

    for (uint32_t z = 0; z < cellsZ; ++z)
    {
        for (uint32_t x = 0; x < cellsX; ++x)
        {
            const uint32_t topLeft = z * vertexGridX + x;
            const uint32_t topRight = topLeft + 1;
            const uint32_t bottomLeft = (z + 1) * vertexGridX + x;
            const uint32_t bottomRight = bottomLeft + 1;

            indices.push_back(bottomRight);
            indices.push_back(topRight);
            indices.push_back(topLeft);

            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
            indices.push_back(topLeft);
        }
    }

    m_IndexCount = static_cast<uint32_t>(indices.size());

    wgpu::Device device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor vertexBufferDescriptor{
        .label = "Sea vertex buffer",
        .usage = wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(VertexP3C3),
        .mappedAtCreation = true
    };
    m_VertexBuffer = device.CreateBuffer(&vertexBufferDescriptor);
    memcpy(m_VertexBuffer.GetMappedRange(), vertices.data(), vertices.size() * sizeof(VertexP3C3));
    m_VertexBuffer.Unmap();

    wgpu::BufferDescriptor indexBufferDescriptor{
        .label = "Sea index buffer",
        .usage = wgpu::BufferUsage::Index,
        .size = indices.size() * sizeof(uint32_t),
        .mappedAtCreation = true
    };
    m_IndexBuffer = device.CreateBuffer(&indexBufferDescriptor);
    memcpy(m_IndexBuffer.GetMappedRange(), indices.data(), indices.size() * sizeof(uint32_t));
    m_IndexBuffer.Unmap();
}

float SeaRenderSystem::ShoreDistanceField::Sample(const std::vector<float>& values, float worldX, float worldZ) const
{
    if (values.empty())
    {
        return 0.0f;
    }

    const float fx = (worldX + Width * CellSize * 0.5f) / CellSize;
    const float fz = (worldZ + Length * CellSize * 0.5f) / CellSize;
    const int x0 = static_cast<int>(glm::floor(fx));
    const int z0 = static_cast<int>(glm::floor(fz));
    const float tx = fx - x0;
    const float tz = fz - z0;

    auto at = [this, &values](int x, int z) -> float {
        x = glm::clamp(x, 0, static_cast<int>(Width) - 1);
        z = glm::clamp(z, 0, static_cast<int>(Length) - 1);
        return values[x + z * Width];
    };

    return glm::mix(
        glm::mix(at(x0, z0), at(x0 + 1, z0), tx),
        glm::mix(at(x0, z0 + 1), at(x0 + 1, z0 + 1), tx),
        tz);
}

SeaRenderSystem::ShoreDistanceField SeaRenderSystem::BuildShoreDistanceField(const LandscapeComponent* pLandscapeComponent) const
{
    ShoreDistanceField field;
    if (!pLandscapeComponent || pLandscapeComponent->Heightmap.empty())
    {
        return field;
    }

    const LandscapeComponent& landscape = *pLandscapeComponent;
    field.Width = landscape.Width * ShoreFieldSupersample;
    field.Length = landscape.Length * ShoreFieldSupersample;
    field.CellSize = landscape.CellSize / ShoreFieldSupersample;

    const int width = static_cast<int>(field.Width);
    const int length = static_cast<int>(field.Length);

    auto getTerrainHeight = [&landscape](int x, int z) -> float {
        x = glm::clamp(x, 0, static_cast<int>(landscape.Width) - 1);
        z = glm::clamp(z, 0, static_cast<int>(landscape.Length) - 1);
        return landscape.Heightmap[x + z * landscape.Width] * landscape.Height - landscape.WaterLevel;
    };

    // Seed the sweep: land samples are the source the distance spreads out from, and everything
    // else starts out unreachably far away.
    constexpr float Unreachable = 1.0e5f;
    std::vector<ShoreSweepCell> grid(field.Width * field.Length);
    for (int z = 0; z < length; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            const float fx = static_cast<float>(x) / ShoreFieldSupersample;
            const float fz = static_cast<float>(z) / ShoreFieldSupersample;
            const int x0 = static_cast<int>(glm::floor(fx));
            const int z0 = static_cast<int>(glm::floor(fz));
            const float tx = fx - x0;
            const float tz = fz - z0;
            const float height = glm::mix(
                glm::mix(getTerrainHeight(x0, z0), getTerrainHeight(x0 + 1, z0), tx),
                glm::mix(getTerrainHeight(x0, z0 + 1), getTerrainHeight(x0 + 1, z0 + 1), tx),
                tz);

            ShoreSweepCell& cell = grid[x + z * width];
            cell.Offset = (height > 0.0f) ? glm::vec2(0.0f) : glm::vec2(Unreachable);
            cell.DistanceSquared = glm::dot(cell.Offset, cell.Offset);
        }
    }

    for (int z = 0; z < length; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            ShoreSweepCell& cell = grid[x + z * width];
            CompareShoreSweepCell(grid, cell, x, z, -1, 0, width, length);
            CompareShoreSweepCell(grid, cell, x, z, 0, -1, width, length);
            CompareShoreSweepCell(grid, cell, x, z, -1, -1, width, length);
            CompareShoreSweepCell(grid, cell, x, z, 1, -1, width, length);
        }
        for (int x = width - 1; x >= 0; --x)
        {
            CompareShoreSweepCell(grid, grid[x + z * width], x, z, 1, 0, width, length);
        }
    }

    for (int z = length - 1; z >= 0; --z)
    {
        for (int x = width - 1; x >= 0; --x)
        {
            ShoreSweepCell& cell = grid[x + z * width];
            CompareShoreSweepCell(grid, cell, x, z, 1, 0, width, length);
            CompareShoreSweepCell(grid, cell, x, z, 0, 1, width, length);
            CompareShoreSweepCell(grid, cell, x, z, 1, 1, width, length);
            CompareShoreSweepCell(grid, cell, x, z, -1, 1, width, length);
        }
        for (int x = 0; x < width; ++x)
        {
            CompareShoreSweepCell(grid, grid[x + z * width], x, z, -1, 0, width, length);
        }
    }

    FastNoiseLite phaseNoise;
    phaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    phaseNoise.SetSeed(static_cast<int>(landscape.Seed));
    phaseNoise.SetFrequency(1.0f / 300.0f); // Roughly the length of coast that shares an arrival time.

    field.Distance.resize(grid.size());
    field.Phase.resize(grid.size());

    const float halfWidth = field.Width * field.CellSize * 0.5f;
    const float halfLength = field.Length * field.CellSize * 0.5f;

    for (int z = 0; z < length; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            const ShoreSweepCell& cell = grid[x + z * width];

            // Half a cell of that distance is the gap between the outermost water sample and the
            // first land one, and the waterline itself falls somewhere in between.
            field.Distance[x + z * width] = glm::max(0.0f, glm::sqrt(cell.DistanceSquared) * field.CellSize - field.CellSize * 0.5f);

            const glm::vec2 shorePoint(
                (x + cell.Offset.x) * field.CellSize - halfWidth,
                (z + cell.Offset.y) * field.CellSize - halfLength);
            field.Phase[x + z * width] = phaseNoise.GetNoise(shorePoint.x, shorePoint.y) * 0.5f + 0.5f;
        }
    }

    return field;
}

void SeaRenderSystem::CreateUniforms()
{
    static_assert(sizeof(SeaUniforms) % 16 == 0);

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Sea uniforms buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(SeaUniforms)
    };
    m_UniformsBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);

    wgpu::BindGroupLayoutEntry bindGroupLayoutEntry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer{
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(SeaUniforms) }
    };
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry
    };
    m_UniformsBindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    wgpu::BindGroupEntry bindGroupEntry{
        .binding = 0,
        .buffer = m_UniformsBuffer,
        .offset = 0,
        .size = sizeof(SeaUniforms)
    };
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_UniformsBindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    m_UniformsBindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void SeaRenderSystem::UpdateUniforms(const SeaComponent& seaComponent)
{
    for (size_t i = 0; i < seaComponent.Waves.size(); ++i)
    {
        const SeaWave& wave = seaComponent.Waves[i];
        const float angle = glm::radians(wave.DirectionAngle);
        m_Uniforms.waves[i] = glm::vec4(glm::cos(angle), glm::sin(angle), wave.Steepness, wave.Wavelength);
    }
    m_Uniforms.shallowColor = seaComponent.ShallowColor;
    m_Uniforms.deepColor = seaComponent.DeepColor;
    m_Uniforms.params0 = glm::vec4(seaComponent.DepthFalloff, seaComponent.FoamBandWidth, seaComponent.FoamSpeed, seaComponent.FoamReach);
    m_Uniforms.params1 = glm::vec4(seaComponent.SpecularStrength, seaComponent.FresnelStrength, seaComponent.SparkleStrength, seaComponent.SparkleScale);
    m_Uniforms.params2 = glm::vec4(seaComponent.Amplitude, seaComponent.Speed, seaComponent.FoamWashWidth, seaComponent.FoamIntensity);
    m_Uniforms.params3 = glm::vec4(seaComponent.FoamWarpScale, seaComponent.FoamWarpStrength, seaComponent.FoamCrestSharpness, 0.0f);

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_UniformsBuffer, 0, &m_Uniforms, sizeof(SeaUniforms));
}

void SeaRenderSystem::CreateRenderPipeline()
{
    if (!m_pShader)
    {
        return;
    }

    wgpu::BlendState blendState{
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
    };

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSceneFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_UniformsBindGroupLayout
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // The sea is translucent: it depth-tests against the terrain but doesn't write depth,
    // so it never occludes anything drawn after it.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Less
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Sea render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = GetRenderSystem()->GetVertexBufferLayout(VertexFormat::VERTEX_FORMAT_P3_C3) },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void SeaRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (GetActiveScene() == nullptr)
    {
        return;
    }

    if (m_RenderPipeline && m_VertexBuffer && m_IndexBuffer && m_IndexCount > 0)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetBindGroup(1, m_UniformsBindGroup);
        renderPass.SetVertexBuffer(0, m_VertexBuffer);
        renderPass.SetIndexBuffer(m_IndexBuffer, wgpu::IndexFormat::Uint32);
        renderPass.DrawIndexed(m_IndexCount);
    }
}

void SeaRenderSystem::DrawDebugUI()
{
    if (!IsDebugUIVisible() || GetActiveScene() == nullptr)
    {
        return;
    }

    entt::registry& registry = GetActiveScene()->GetRegistry();
    SeaComponent* pSeaComponent = nullptr;
    auto seaView = registry.view<SeaComponent>();
    for (const auto entity : seaView)
    {
        pSeaComponent = &seaView.get<SeaComponent>(entity);
        break;
    }

    if (!pSeaComponent)
    {
        return;
    }

    ImGui::Begin("Sea", &m_ShowDebugUI, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushItemWidth(300);

    ImGui::SeparatorText("Waves");
    for (size_t i = 0; i < pSeaComponent->Waves.size(); ++i)
    {
        SeaWave& wave = pSeaComponent->Waves[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("Wave %zu", i + 1);
        ImGui::DragFloat("Direction", &wave.DirectionAngle, 1.0f, 0.0f, 360.0f, "%.0f");
        ImGui::DragFloat("Wavelength", &wave.Wavelength, 1.0f, 5.0f, 500.0f, "%.0f");
        ImGui::DragFloat("Steepness", &wave.Steepness, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
    }
    ImGui::DragFloat("Amplitude", &pSeaComponent->Amplitude, 0.01f, 0.0f, 3.0f, "%.2f");
    ImGui::DragFloat("Speed", &pSeaComponent->Speed, 0.01f, 0.0f, 3.0f, "%.2f");

    ImGui::SeparatorText("Color");
    ImGui::ColorEdit4("Shallow", &pSeaComponent->ShallowColor.x);
    ImGui::ColorEdit4("Deep", &pSeaComponent->DeepColor.x);
    ImGui::DragFloat("Depth falloff", &pSeaComponent->DepthFalloff, 1.0f, 1.0f, 200.0f, "%.0f");

    ImGui::SeparatorText("Foam");
    ImGui::DragFloat("Band width", &pSeaComponent->FoamBandWidth, 0.5f, 2.0f, 120.0f, "%.1f");
    ImGui::DragFloat("Foam speed", &pSeaComponent->FoamSpeed, 0.1f, 0.0f, 40.0f, "%.1f");
    ImGui::DragFloat("Reach", &pSeaComponent->FoamReach, 1.0f, 0.0f, 400.0f, "%.0f");
    ImGui::DragFloat("Wash width", &pSeaComponent->FoamWashWidth, 0.1f, 0.0f, 40.0f, "%.1f");
    ImGui::DragFloat("Warp scale", &pSeaComponent->FoamWarpScale, 0.001f, 0.001f, 0.2f, "%.3f");
    ImGui::DragFloat("Warp strength", &pSeaComponent->FoamWarpStrength, 0.1f, 0.0f, 40.0f, "%.1f");
    ImGui::DragFloat("Crest sharpness", &pSeaComponent->FoamCrestSharpness, 0.01f, 0.02f, 1.0f, "%.2f");
    ImGui::DragFloat("Intensity", &pSeaComponent->FoamIntensity, 0.01f, 0.0f, 1.0f, "%.2f");

    ImGui::SeparatorText("Shading");
    ImGui::DragFloat("Specular", &pSeaComponent->SpecularStrength, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Fresnel", &pSeaComponent->FresnelStrength, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Sparkle", &pSeaComponent->SparkleStrength, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Sparkle scale", &pSeaComponent->SparkleScale, 0.005f, 0.005f, 2.0f, "%.3f");

    ImGui::SeparatorText("Grid");
    if (ImGui::DragFloat("Cell size", &pSeaComponent->CellSize, 1.0f, 5.0f, 100.0f, "%.0f"))
    {
        pSeaComponent->Generation++;
    }

    ImGui::PopItemWidth();
    ImGui::End();
}

void SeaRenderSystem::HandleShaderInjection()
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
