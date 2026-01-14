#pragma once

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

// Atmospheric scattering component using Sean O'Neil's algorithm
// Reference: GPU Gems 2, Chapter 16 - Accurate Atmospheric Scattering
class AtmosphereComponent : public IComponent
{
public:
    AtmosphereComponent() {}
    ~AtmosphereComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    // Rayleigh scattering constant - controls overall scattering strength
    // Higher values = more scattering, bluer sky
    float Kr{ 0.0025f };

    // Mie scattering constant - controls aerosol/haze scattering
    // Higher values = more haze, brighter sun glow
    float Km{ 0.0010f };

    // Sun brightness multiplier
    float ESun{ 20.0f };

    // Mie phase asymmetry factor (-0.75 to -0.999)
    // More negative = tighter sun glow
    float g{ -0.990f };

    // Wavelengths for RGB channels (in micrometers)
    // Default produces Earth-like blue sky
    glm::vec3 wavelength{ 0.650f, 0.570f, 0.475f };

    // Scale depth - fraction of atmosphere height where average density is found
    // 0.25 means average density is at 25% of atmosphere thickness
    float scaleDepth{ 0.25f };

    // Number of samples for ray marching (5-10 typical)
    // More samples = better quality but slower
    int numSamples{ 5 };

    // GPU resources
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;

    bool initialized{ false };
};

REGISTER_COMPONENT(AtmosphereComponent, "atmosphere")

} // namespace WingsOfSteel
