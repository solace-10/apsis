#pragma once

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class AtmosphereComponent : public IComponent
{
public:
    AtmosphereComponent() {}
    ~AtmosphereComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    // Atmosphere thickness in kilometers (extends outward from planet surface)
    float height{ 100.0f };

    // Rayleigh scattering color (typically blueish for Earth-like atmospheres)
    glm::vec3 color{ 0.4f, 0.6f, 1.0f };

    // Density falloff control (higher = thinner atmosphere at edges)
    float density{ 1.0f };

    // Uniform buffer for shader parameters
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;

    bool initialized{ false };
};

REGISTER_COMPONENT(AtmosphereComponent, "atmosphere")

} // namespace WingsOfSteel
