#pragma once

#include <cstdint>
#include <vector>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel
{

class PlanetComponent;
struct VertexP3N3UV;

class PlanetMeshGenerator
{
public:
    static void Generate(PlanetComponent& component, uint32_t subdivisions = 16);

private:
    static void GenerateSubdivisions(
        std::vector<VertexP3N3UV>& vertices,
        std::vector<uint32_t>& indices,
        float semiMajorRadius,
        float semiMinorRadius,
        uint32_t subdivisions);

    static void FixUVSeams(
        std::vector<VertexP3N3UV>& vertices,
        std::vector<uint32_t>& indices);

    static void GenerateWireframe(
        PlanetComponent& component,
        const std::vector<VertexP3N3UV>& vertices,
        const std::vector<uint32_t>& indices,
        wgpu::Device device);
};

} // namespace WingsOfSteel
