#include "planet_mesh_generator.hpp"

#include <vector>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/vertex_types.hpp>

#include "components/planet_component.hpp"

namespace WingsOfSteel
{

void PlanetMeshGenerator::Generate(PlanetComponent& component)
{
    if (component.initialized)
    {
        return;
    }

    const float halfExtent = 2.5f;
    const glm::vec3 color(0.4f, 0.6f, 0.3f); // Earthy green

    // 24 vertices (4 per face for proper face normals)
    // 6 faces: +X, -X, +Y, -Y, +Z, -Z
    std::vector<VertexP3C3N3> vertices = {
        // +X face (right)
        { { halfExtent, -halfExtent, -halfExtent }, color, { 1.0f, 0.0f, 0.0f } },
        { { halfExtent, halfExtent, -halfExtent }, color, { 1.0f, 0.0f, 0.0f } },
        { { halfExtent, halfExtent, halfExtent }, color, { 1.0f, 0.0f, 0.0f } },
        { { halfExtent, -halfExtent, halfExtent }, color, { 1.0f, 0.0f, 0.0f } },

        // -X face (left)
        { { -halfExtent, -halfExtent, halfExtent }, color, { -1.0f, 0.0f, 0.0f } },
        { { -halfExtent, halfExtent, halfExtent }, color, { -1.0f, 0.0f, 0.0f } },
        { { -halfExtent, halfExtent, -halfExtent }, color, { -1.0f, 0.0f, 0.0f } },
        { { -halfExtent, -halfExtent, -halfExtent }, color, { -1.0f, 0.0f, 0.0f } },

        // +Y face (top)
        { { -halfExtent, halfExtent, -halfExtent }, color, { 0.0f, 1.0f, 0.0f } },
        { { -halfExtent, halfExtent, halfExtent }, color, { 0.0f, 1.0f, 0.0f } },
        { { halfExtent, halfExtent, halfExtent }, color, { 0.0f, 1.0f, 0.0f } },
        { { halfExtent, halfExtent, -halfExtent }, color, { 0.0f, 1.0f, 0.0f } },

        // -Y face (bottom)
        { { -halfExtent, -halfExtent, halfExtent }, color, { 0.0f, -1.0f, 0.0f } },
        { { -halfExtent, -halfExtent, -halfExtent }, color, { 0.0f, -1.0f, 0.0f } },
        { { halfExtent, -halfExtent, -halfExtent }, color, { 0.0f, -1.0f, 0.0f } },
        { { halfExtent, -halfExtent, halfExtent }, color, { 0.0f, -1.0f, 0.0f } },

        // +Z face (front)
        { { -halfExtent, -halfExtent, halfExtent }, color, { 0.0f, 0.0f, 1.0f } },
        { { halfExtent, -halfExtent, halfExtent }, color, { 0.0f, 0.0f, 1.0f } },
        { { halfExtent, halfExtent, halfExtent }, color, { 0.0f, 0.0f, 1.0f } },
        { { -halfExtent, halfExtent, halfExtent }, color, { 0.0f, 0.0f, 1.0f } },

        // -Z face (back)
        { { halfExtent, -halfExtent, -halfExtent }, color, { 0.0f, 0.0f, -1.0f } },
        { { -halfExtent, -halfExtent, -halfExtent }, color, { 0.0f, 0.0f, -1.0f } },
        { { -halfExtent, halfExtent, -halfExtent }, color, { 0.0f, 0.0f, -1.0f } },
        { { halfExtent, halfExtent, -halfExtent }, color, { 0.0f, 0.0f, -1.0f } },
    };

    // 36 indices (6 faces * 2 triangles * 3 indices)
    std::vector<uint32_t> indices = {
        // +X face
        0,
        1,
        2,
        0,
        2,
        3,
        // -X face
        4,
        5,
        6,
        4,
        6,
        7,
        // +Y face
        8,
        9,
        10,
        8,
        10,
        11,
        // -Y face
        12,
        13,
        14,
        12,
        14,
        15,
        // +Z face
        16,
        17,
        18,
        16,
        18,
        19,
        // -Z face
        20,
        21,
        22,
        20,
        22,
        23,
    };

    component.vertexCount = static_cast<uint32_t>(vertices.size());
    component.indexCount = static_cast<uint32_t>(indices.size());

    wgpu::Device device = GetRenderSystem()->GetDevice();

    // Create vertex buffer
    {
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Planet vertex buffer",
            .usage = wgpu::BufferUsage::Vertex,
            .size = vertices.size() * sizeof(VertexP3C3N3),
            .mappedAtCreation = true
        };
        component.vertexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(component.vertexBuffer.GetMappedRange(), vertices.data(), vertices.size() * sizeof(VertexP3C3N3));
        component.vertexBuffer.Unmap();
    }

    // Create index buffer
    {
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Planet index buffer",
            .usage = wgpu::BufferUsage::Index,
            .size = indices.size() * sizeof(uint32_t),
            .mappedAtCreation = true
        };
        component.indexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(component.indexBuffer.GetMappedRange(), indices.data(), indices.size() * sizeof(uint32_t));
        component.indexBuffer.Unmap();
    }

    // Generate wireframe mesh with barycentric coordinates (unindexed)
    // Each triangle gets 3 vertices with barycentric coords (1,0,0), (0,1,0), (0,0,1)
    {
        std::vector<VertexP3B3> wireframeVertices;
        wireframeVertices.reserve(indices.size());

        const glm::vec3 bary0(1.0f, 0.0f, 0.0f);
        const glm::vec3 bary1(0.0f, 1.0f, 0.0f);
        const glm::vec3 bary2(0.0f, 0.0f, 1.0f);

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            const glm::vec3& p0 = vertices[indices[i]].position;
            const glm::vec3& p1 = vertices[indices[i + 1]].position;
            const glm::vec3& p2 = vertices[indices[i + 2]].position;

            wireframeVertices.push_back({ p0, bary0 });
            wireframeVertices.push_back({ p1, bary1 });
            wireframeVertices.push_back({ p2, bary2 });
        }

        component.wireframeVertexCount = static_cast<uint32_t>(wireframeVertices.size());

        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Planet wireframe vertex buffer",
            .usage = wgpu::BufferUsage::Vertex,
            .size = wireframeVertices.size() * sizeof(VertexP3B3),
            .mappedAtCreation = true
        };
        component.wireframeVertexBuffer = device.CreateBuffer(&bufferDescriptor);
        memcpy(component.wireframeVertexBuffer.GetMappedRange(), wireframeVertices.data(), wireframeVertices.size() * sizeof(VertexP3B3));
        component.wireframeVertexBuffer.Unmap();
    }

    component.initialized = true;
}

} // namespace WingsOfSteel
