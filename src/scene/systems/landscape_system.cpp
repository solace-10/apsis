#include "scene/systems/landscape_system.hpp"

#include <vector>

#include <FastNoiseLite.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui/imgui.hpp"
#include "pandora.hpp"
#include "physics/collision_shape.hpp"
#include "resources/resource_texture_2d.hpp"
#include "scene/components/landscape_component.hpp"
#include "scene/components/rigid_body_component.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

LandscapeSystem::LandscapeSystem()
{
}

LandscapeSystem::~LandscapeSystem()
{
}

void LandscapeSystem::Update(float delta)
{
}

void LandscapeSystem::Generate(EntitySharedPtr pLandscapeEntity)
{
    if (!pLandscapeEntity->HasComponent<LandscapeComponent>())
    {
        Log::Error() << "Entity has no LandscapeComponent.";
        return;
    }

    m_pLandscapeEntity = pLandscapeEntity;
    LandscapeComponent& landscapeComponent = pLandscapeEntity->GetComponent<LandscapeComponent>();
    GenerateInternal(landscapeComponent);
}

void LandscapeSystem::GenerateInternal(LandscapeComponent& landscapeComponent)
{
    const size_t heightmapSize = landscapeComponent.Width * landscapeComponent.Length;
    if (landscapeComponent.Heightmap.size() != heightmapSize)
    {
        landscapeComponent.Heightmap.resize(heightmapSize);
    }

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(landscapeComponent.Octaves);
    noise.SetFrequency(landscapeComponent.Frequency);
    noise.SetSeed(landscapeComponent.Seed);

    // The warp uses an offset seed, so the distortion doesn't correlate with the terrain it is
    // being applied to, which would largely cancel it out.
    FastNoiseLite warp;
    warp.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    warp.SetDomainWarpAmp(landscapeComponent.DomainWarpAmp);
    warp.SetFrequency(landscapeComponent.DomainWarpFrequency);
    warp.SetSeed(landscapeComponent.Seed + 1337);

    FastNoiseLite continentNoise;
    continentNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentNoise.SetFractalOctaves(2);
    continentNoise.SetFrequency(landscapeComponent.ContinentFrequency);
    continentNoise.SetSeed(landscapeComponent.Seed + 7919);

    FastNoiseLite ridgeNoise;
    ridgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    ridgeNoise.SetFractalOctaves(landscapeComponent.Octaves);
    ridgeNoise.SetFrequency(landscapeComponent.RidgeFrequency);
    ridgeNoise.SetSeed(landscapeComponent.Seed + 4441);

    for (int y = 0; y < landscapeComponent.Length; y++)
    {
        for (int x = 0; x < landscapeComponent.Width; x++)
        {
            // Only the terrain sample is warped, never the coordinates the falloff is based on.
            // Warping both would tie the shape of the coastline to the terrain and the two would
            // largely cancel out.
            float warpedX = static_cast<float>(x);
            float warpedY = static_cast<float>(y);
            warp.DomainWarp(warpedX, warpedY);

            float height = noise.GetNoise(warpedX, warpedY);
            height = (height + 1.0f) * 0.5f; // Remap from [-1, 1] to [0, 1]

            // Scale the terrain by a large scale, slowly varying noise, so land clumps together
            // rather than being spread evenly across the map. The unwarped coordinates are used,
            // as this is meant to be a broad shape rather than terrain detail.
            float continent = continentNoise.GetNoise(static_cast<float>(x), static_cast<float>(y));
            continent = (continent + 1.0f) * 0.5f;
            height *= glm::mix(1.0f, continent, landscapeComponent.ContinentStrength);

            // Push the terrain down towards the edges of the map, so we end up with an island
            // rather than terrain which runs off in every direction.
            const float distance = glm::length(NormalizedCoords(landscapeComponent, x, y));
            const float falloff = glm::smoothstep(landscapeComponent.FalloffStart, landscapeComponent.FalloffEnd, distance);
            height = glm::clamp(height - falloff * landscapeComponent.FalloffStrength, 0.0f, 1.0f);

            // Flatten out the lower ground while leaving the peaks mostly untouched, which widens
            // the beaches and gives us coastal plains to build on.
            height = glm::pow(height, landscapeComponent.Redistribution);

            // Ridges are weighted by the inverse of the falloff, so they build up towards the
            // middle of the island and leave the coastline gentle.
            if (landscapeComponent.RidgeStrength > 0.0f)
            {
                float ridge = ridgeNoise.GetNoise(warpedX, warpedY);
                ridge = (ridge + 1.0f) * 0.5f;
                height = glm::clamp(height + ridge * landscapeComponent.RidgeStrength * (1.0f - falloff), 0.0f, 1.0f);
            }

            landscapeComponent.Heightmap[y * landscapeComponent.Width + x] = height;
        }
    }

    landscapeComponent.Generation++;

    GenerateDebugHeightmapTexture(landscapeComponent);

    if (EntitySharedPtr pLandscapeEntity = m_pLandscapeEntity.lock())
    {
        BuildPhysics(pLandscapeEntity, landscapeComponent);
    }
}

// Maps a cell into [-1, 1] on both axes, with (0, 0) at the center of the map. Cell centers are
// used, so the map is sampled symmetrically regardless of its dimensions.
glm::vec2 LandscapeSystem::NormalizedCoords(const LandscapeComponent& landscapeComponent, int x, int y) const
{
    return glm::vec2(
        (static_cast<float>(x) + 0.5f) / static_cast<float>(landscapeComponent.Width) * 2.0f - 1.0f,
        (static_cast<float>(y) + 0.5f) / static_cast<float>(landscapeComponent.Length) * 2.0f - 1.0f);
}

void LandscapeSystem::BuildPhysics(EntitySharedPtr pLandscapeEntity, const LandscapeComponent& landscapeComponent)
{
    // Build a physics heightfield that matches the visual mesh exactly. This mirrors the
    // vertex generation in LandscapeRenderSystem::GenerateGeometry: a (Width+1) x (Length+1)
    // grid of points, sampling the same clamped heights and using the same shared CellSize.
    const uint32_t gridWidth = landscapeComponent.Width + 1;
    const uint32_t gridLength = landscapeComponent.Length + 1;
    const float maxHeight = landscapeComponent.Height;
    const float waterLevel = landscapeComponent.WaterLevel;

    auto getHeight = [&landscapeComponent, maxHeight, waterLevel](int x, int z) -> float {
        x = glm::clamp(x, 0, static_cast<int>(landscapeComponent.Width) - 1);
        z = glm::clamp(z, 0, static_cast<int>(landscapeComponent.Length) - 1);
        return landscapeComponent.Heightmap[x + z * landscapeComponent.Width] * maxHeight - waterLevel;
    };

    // Row-major, x fastest, to match btHeightfieldTerrainShape's expected data layout.
    std::vector<float> heights;
    heights.reserve(static_cast<size_t>(gridWidth) * gridLength);
    for (uint32_t z = 0; z < gridLength; ++z)
    {
        for (uint32_t x = 0; x < gridWidth; ++x)
        {
            heights.push_back(getHeight(static_cast<int>(x), static_cast<int>(z)));
        }
    }

    auto pShape = std::make_shared<CollisionShapeHeightfield>(gridWidth, gridLength, std::move(heights), landscapeComponent.CellSize);

    // Bullet centres the heightfield vertically; lift the body so the raw heights line up with
    // the rendered mesh (which places vertices at their world-space height directly).
    const glm::mat4x4 worldTransform = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, pShape->GetVerticalOffset(), 0.0f));

    // Rebuild from scratch on regeneration so the previous body is removed from the world.
    if (pLandscapeEntity->HasComponent<RigidBodyComponent>())
    {
        pLandscapeEntity->RemoveComponent<RigidBodyComponent>();
    }

    RigidBodyComponent& rigidBodyComponent = pLandscapeEntity->AddComponent<RigidBodyComponent>();
    rigidBodyComponent.CreateStatic(pShape, worldTransform);
    rigidBodyComponent.SetOwner(pLandscapeEntity);
}

void LandscapeSystem::GenerateDebugHeightmapTexture(LandscapeComponent& landscapeComponent)
{
    std::vector<uint8_t> textureData;
    textureData.resize(landscapeComponent.Width * landscapeComponent.Length * 4);

    for (size_t i = 0; i < landscapeComponent.Heightmap.size(); ++i)
    {
        uint8_t height = static_cast<uint8_t>(landscapeComponent.Heightmap[i] * 255.0f);
        size_t index = i * 4;
        textureData[index] = height;
        textureData[index + 1] = height;
        textureData[index + 2] = height;
        textureData[index + 3] = 255;
    }
    landscapeComponent.DebugHeightmapTexture = std::make_unique<ResourceTexture2D>("Heightmap", ColorSpace::Linear, textureData.data(), textureData.size(), landscapeComponent.Width, landscapeComponent.Length, 4);
}

void LandscapeSystem::DrawDebugUI()
{
    if (!IsDebugUIVisible())
    {
        return;
    }

    EntitySharedPtr pLandscapeEntity = m_pLandscapeEntity.lock();
    if (!pLandscapeEntity)
    {
        return;
    }

    LandscapeComponent& landscapeComponent = pLandscapeEntity->GetComponent<LandscapeComponent>();

    ImGui::Begin("Landscape generator", &m_ShowDebugUI, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::BeginGroup();
    ImGui::PushItemWidth(300);

    int seed = static_cast<int>(landscapeComponent.Seed);
    if (ImGui::DragInt("Seed", &seed, 1.0f, 0, 65536))
    {
        landscapeComponent.Seed = static_cast<uint32_t>(seed);
    }

    int landscapeSize = static_cast<int>(landscapeComponent.Width);
    if (ImGui::DragInt("Size", &landscapeSize, 1.0f, 32, 1024))
    {
        landscapeComponent.Width = static_cast<uint32_t>(landscapeSize);
        landscapeComponent.Length = static_cast<uint32_t>(landscapeSize);
    }

    ImGui::DragFloat("Height", &landscapeComponent.Height, 1.0f, 8.0f, 500.0f, "%.2f");
    ImGui::DragFloat("Water level", &landscapeComponent.WaterLevel, 1.0f, 0.0f, 500.0f, "%.2f");

    int octaves = static_cast<int>(landscapeComponent.Octaves);
    if (ImGui::DragInt("Octaves", &octaves, 1.0f, 1, 10))
    {
        landscapeComponent.Octaves = static_cast<uint32_t>(octaves);
    }

    ImGui::DragFloat("Frequency", &landscapeComponent.Frequency, 0.0001f, 0.0f, 0.03f, "%.4f");

    ImGui::SeparatorText("Island");

    ImGui::DragFloat("Falloff start", &landscapeComponent.FalloffStart, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Falloff end", &landscapeComponent.FalloffEnd, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Falloff strength", &landscapeComponent.FalloffStrength, 0.01f, 0.0f, 2.0f, "%.2f");
    ImGui::DragFloat("Domain warp amp", &landscapeComponent.DomainWarpAmp, 0.5f, 0.0f, 200.0f, "%.1f");
    ImGui::DragFloat("Domain warp frequency", &landscapeComponent.DomainWarpFrequency, 0.0005f, 0.0f, 0.1f, "%.4f");
    ImGui::DragFloat("Continent frequency", &landscapeComponent.ContinentFrequency, 0.0001f, 0.0f, 0.02f, "%.4f");
    ImGui::DragFloat("Continent strength", &landscapeComponent.ContinentStrength, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Redistribution", &landscapeComponent.Redistribution, 0.01f, 0.1f, 4.0f, "%.2f");
    ImGui::DragFloat("Ridge strength", &landscapeComponent.RidgeStrength, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Ridge frequency", &landscapeComponent.RidgeFrequency, 0.0005f, 0.0f, 0.1f, "%.4f");

    if (ImGui::Button("Generate"))
    {
        GenerateInternal(landscapeComponent);
    }
    ImGui::PopItemWidth();
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    if (landscapeComponent.DebugHeightmapTexture)
    {
        ImGui::Image(landscapeComponent.DebugHeightmapTexture->GetTextureView(), ImVec2(512, 512));
    }
    ImGui::EndGroup();

    ImGui::End();
}

} // namespace WingsOfSteel
