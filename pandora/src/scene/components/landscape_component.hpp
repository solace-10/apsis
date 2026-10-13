#pragma once

#include <array>
#include <vector>

#include <glm/vec3.hpp>

#include "core/serialization.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"
#include "resources/resource.fwd.hpp"
#include "resources/resource_texture_2d.hpp"

namespace WingsOfSteel
{

class LandscapeComponent : public IComponent
{
public:
    LandscapeComponent() = default;
    ~LandscapeComponent() = default;

    uint32_t Seed{ 0 };
    uint32_t Width{ 32 };
    uint32_t Length{ 32 };
    float Height{ 40.0f }; // Maximum height the landscape will be generated to, in meters.
    float WaterLevel{ 10.0f };
    float CellSize{ 10.0f }; // Cell size, in meters. Shared by the visual mesh and the physics heightfield.
    uint32_t Generation{ 0 }; // Current generation iteration; used by the renderer to know when the component is stale.
    uint32_t Octaves{ 4 };
    float Frequency{ 0.002 };

    // Island shaping. The falloff mask is subtracted from the terrain height, rather than
    // scaling it, so relief is preserved and the coastline falls wherever the noise happens
    // to cross the water level.
    float FalloffStart{ 0.35f }; // Normalized distance from the center at which land starts being pushed down.
    float FalloffEnd{ 0.95f }; // Normalized distance at which the falloff reaches full strength.
    float FalloffStrength{ 1.0f }; // How much height is subtracted at full falloff, in normalized height.

    // Domain warping distorts the terrain before the falloff is applied, turning what would
    // otherwise be a fairly circular island into one with headlands and inlets.
    float DomainWarpAmp{ 30.0f }; // Warp distance, in cells. Set to 0 to disable warping.
    float DomainWarpFrequency{ 0.01f };

    // Continentalness biases one broad region of the map upwards, so that the terrain naturally
    // clumps into a single landmass instead of an even spread of land and water.
    float ContinentFrequency{ 0.002f };
    float ContinentStrength{ 0.5f }; // How much the continent noise is blended in. 0 disables it.

    // Redistribution flattens the lowlands while leaving the peaks alone, giving us wide beaches
    // and coastal plains. Values above 1 flatten, values below 1 raise the terrain.
    float Redistribution{ 1.6f };

    // An optional ridged fractal, blended in towards the middle of the island to give it a
    // mountainous spine. Disabled by default.
    float RidgeStrength{ 0.0f };
    float RidgeFrequency{ 0.02f };

    std::vector<float> Heightmap;
    ResourceTexture2DUniquePtr DebugHeightmapTexture;

    // Terrain splatting. Layer i tints the ground with SplatColors[i] in proportion to its weight
    // at that point; whatever weight is left over falls back to SplatBaseColor, so a cell nothing
    // has claimed stays bare terrain. Whoever owns the landscape fills these in, as with the
    // heightmap.
    static constexpr uint32_t kMaxSplatLayers = 8;

    // Per-cell layer weights, quantized to bytes. Laid out as kMaxSplatLayers / 4 planes of
    // Width * Length RGBA texels, rather than interleaved per cell: that is precisely the layout
    // an array texture expects, so the upload is a single WriteTexture with no shuffling in
    // between. Empty leaves the landscape uniformly SplatBaseColor.
    //
    // The generation is tracked separately from the one above, as the weights change constantly
    // without the terrain itself being regenerated.
    std::vector<uint8_t> SplatWeights;
    uint32_t SplatWeightsGeneration{ 0 };

    // Cheap enough to be pushed to the GPU every frame, so changing one of these costs nothing
    // and needs no generation of its own.
    std::array<glm::vec3, kMaxSplatLayers> SplatColors{};
    glm::vec3 SplatBaseColor{ 0.72f, 0.58f, 0.41f }; // Light brown.

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Seed = Json::DeserializeUnsignedInteger(pContext, json, "seed", 0);
    }
};

REGISTER_COMPONENT(LandscapeComponent, "landscape")

} // namespace WingsOfSteel
