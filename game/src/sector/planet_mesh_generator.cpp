#include "planet_mesh_generator.hpp"

#include <array>
#include <vector>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/vertex_types.hpp>

#include "components/planet_component.hpp"

namespace WingsOfSteel
{

// Face definition for cube-to-sphere projection
struct CubeFace
{
    glm::vec3 origin; // Center of the face on unit cube
    glm::vec3 uAxis; // Tangent direction for U
    glm::vec3 vAxis; // Tangent direction for V
};

// 6 faces of the cube, each defined by origin and UV tangent directions
// The cube spans from -1 to +1 on each axis
// UV axes chosen so cross(uAxis, vAxis) points outward (same direction as origin)
static const std::array<CubeFace, 6> kCubeFaces = { {
    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // +X: cross(+Y, +Z) = +X
    { { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // -X: cross(+Y, -Z) = -X
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // +Y: cross(+Z, +X) = +Y
    { { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } }, // -Y: cross(-Z, +X) = -Y
    { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // +Z: cross(+X, +Y) = +Z
    { { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // -Z: cross(-X, +Y) = -Z
} };

void PlanetMeshGenerator::Generate(PlanetComponent& component, uint32_t subdivisions)
{
    if (component.initialized)
    {
        return;
    }

    const float semiMajorRadius = component.semiMajorRadius; // Equatorial (X, Z)
    const float semiMinorRadius = component.semiMinorRadius; // Polar (Y)
    const glm::vec3 color(0.4f, 0.6f, 0.3f); // Earthy green

    // Precompute squared radii for normal calculation
    const float semiMajorRadiusSq = semiMajorRadius * semiMajorRadius;
    const float semiMinorRadiusSq = semiMinorRadius * semiMinorRadius;

    const uint32_t vertsPerFace = (subdivisions + 1) * (subdivisions + 1);
    const uint32_t indicesPerFace = subdivisions * subdivisions * 6;

    std::vector<VertexP3C3N3> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(6 * vertsPerFace);
    indices.reserve(6 * indicesPerFace);

    // Generate vertices and indices for each face
    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
        const CubeFace& face = kCubeFaces[faceIndex];
        const uint32_t faceVertexOffset = faceIndex * vertsPerFace;

        // Generate vertices for this face
        for (uint32_t v = 0; v <= subdivisions; ++v)
        {
            for (uint32_t u = 0; u <= subdivisions; ++u)
            {
                // Map (u, v) to [-1, 1] range on the face
                float uNorm = (static_cast<float>(u) / static_cast<float>(subdivisions)) * 2.0f - 1.0f;
                float vNorm = (static_cast<float>(v) / static_cast<float>(subdivisions)) * 2.0f - 1.0f;

                // Calculate position on cube face
                glm::vec3 cubePos = face.origin + uNorm * face.uAxis + vNorm * face.vAxis;

                // Project onto oblate spheroid by normalizing direction and scaling each axis
                glm::vec3 dir = glm::normalize(cubePos);
                glm::vec3 spheroidPos = glm::vec3(
                    dir.x * semiMajorRadius, // X - equatorial
                    dir.y * semiMinorRadius, // Y - polar
                    dir.z * semiMajorRadius // Z - equatorial
                );

                // For an ellipsoid, the normal is NOT the normalized position.
                // The correct normal is the gradient of the implicit surface:
                // F(x,y,z) = (x/a)² + (y/b)² + (z/a)² - 1 = 0
                // ∇F = (2x/a², 2y/b², 2z/a²), normalized
                glm::vec3 normal = glm::normalize(glm::vec3(
                    spheroidPos.x / semiMajorRadiusSq,
                    spheroidPos.y / semiMinorRadiusSq,
                    spheroidPos.z / semiMajorRadiusSq));

                vertices.push_back({ spheroidPos, color, normal });
            }
        }

        // Generate indices for this face (two triangles per quad)
        for (uint32_t v = 0; v < subdivisions; ++v)
        {
            for (uint32_t u = 0; u < subdivisions; ++u)
            {
                uint32_t topLeft = faceVertexOffset + v * (subdivisions + 1) + u;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = topLeft + (subdivisions + 1);
                uint32_t bottomRight = bottomLeft + 1;

                // First triangle (counter-clockwise when viewed from outside)
                indices.push_back(topLeft);
                indices.push_back(bottomRight);
                indices.push_back(bottomLeft);

                // Second triangle
                indices.push_back(topLeft);
                indices.push_back(topRight);
                indices.push_back(bottomRight);
            }
        }
    }

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
