#pragma once

#include <webgpu/webgpu_cpp.h>

#include <core/smart_ptr.hpp>
#include <resources/resource_texture_2d.hpp>
#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{
DECLARE_SMART_PTR(Entity);
}

namespace WingsOfSteel
{

class PlanetComponent : public IComponent
{
public:
    PlanetComponent() {}
    ~PlanetComponent() {}

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

    // Oblate spheroid dimensions (in kilometers)
    float semiMajorRadius{ 1.0f }; // Equatorial radius (X, Z axes)
    float semiMinorRadius{ 1.0f }; // Polar radius (Y axis)

    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    uint32_t vertexCount{ 0 };
    uint32_t indexCount{ 0 };

    // Wireframe mesh (unindexed, with barycentric coordinates)
    wgpu::Buffer wireframeVertexBuffer;
    uint32_t wireframeVertexCount{ 0 };

    // Texture for the planet surface
    ResourceTexture2DSharedPtr colorTexture;
    wgpu::BindGroup textureBindGroup;

    bool initialized{ false };
};

REGISTER_COMPONENT(PlanetComponent, "planet")

} // namespace WingsOfSteel
