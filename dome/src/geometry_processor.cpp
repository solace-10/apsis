#include "geometry_processor.hpp"

#include <delaunator.hpp>
#include <tiny_gltf.h>

#include "geometry_types.hpp"

namespace WingsOfSteel::Dome
{

std::vector<Vertex> GeometryProcessor::GetUniqueVertices(const std::vector<Vertex>& vertices, float threshold) const
{
    std::vector<Vertex> uniqueVertices;

    // Add first vertex as it's always unique
    if (!vertices.empty())
    {
        uniqueVertices.push_back(vertices[0]);
    }

    // For each remaining vertex, check if it's far enough from all existing unique vertices
    for (size_t i = 1; i < vertices.size(); i++) 
    {
        bool isUnique = true;
        const Vertex& candidate = vertices[i];

        // Compare against all existing unique vertices
        for (const Vertex& existing : uniqueVertices) 
        {
            // Calculate distance between points
            float dx = candidate.x - existing.x;
            float dy = candidate.y - existing.y;
            float distSquared = dx * dx + dy * dy;

            // If distance is less than threshold, this vertex is too close to an existing one
            if (distSquared < threshold * threshold) 
            {
                isUnique = false;
                break;
            }
        }

        // If vertex was far enough from all existing vertices, add it
        if (isUnique) 
        {
            uniqueVertices.push_back(candidate);
        }
    }

    return uniqueVertices;
}

std::vector<IndexedTriangle> GeometryProcessor::GetTriangles(const std::vector<Vertex>& vertices) const
{
    std::vector<IndexedTriangle> indexedTriangles;

    std::vector<double> coords;
    for (const auto& vertex : vertices)
    {
        coords.push_back(vertex.x);
        coords.push_back(vertex.y);
    }

    delaunator::Delaunator delaunator(coords);

    for (std::size_t i = 0; i < delaunator.triangles.size(); i += 3)
    {
        IndexedTriangle indexedTriangle;
        indexedTriangle.v0 = delaunator.triangles[i];
        indexedTriangle.v1 = delaunator.triangles[i + 1];
        indexedTriangle.v2 = delaunator.triangles[i + 2];
        indexedTriangles.push_back(indexedTriangle);
    }

    return indexedTriangles;
}

void GeometryProcessor::Export(const std::vector<Vertex>& vertices, const std::vector<IndexedTriangle>& triangles) const
{
    tinygltf::Model model;
    tinygltf::Scene scene;

    // Define your vertex data (x, y, z, r, g, b, a)
    std::vector<float> vertexPositionData;
    std::vector<float> vertexColorData;

    const float scale = 1.0f / 1024.0f * 10.0f;

    for (const auto& vertex : vertices)
    {
        vertexPositionData.push_back(static_cast<float>(vertex.x) * scale);
        vertexPositionData.push_back(static_cast<float>(vertex.y) * scale);
        vertexPositionData.push_back(0.0f);
        vertexColorData.push_back(vertex.color.r);
        vertexColorData.push_back(vertex.color.g);
        vertexColorData.push_back(vertex.color.b);
        vertexColorData.push_back(0.0f);
    }

    // Define your index data (triangle)
    std::vector<uint16_t> indexData;
    for (const auto& triangle : triangles)
    {
        indexData.push_back(static_cast<uint16_t>(triangle.v0));
        indexData.push_back(static_cast<uint16_t>(triangle.v1));
        indexData.push_back(static_cast<uint16_t>(triangle.v2));
    }

    // Create a buffer and fill it with vertex position data
    tinygltf::Buffer buffer;
    buffer.data.insert(buffer.data.end(),
                       reinterpret_cast<const unsigned char*>(vertexPositionData.data()),
                       reinterpret_cast<const unsigned char*>(vertexPositionData.data() + vertexPositionData.size()));

    // Add color data to the buffer
    const size_t colorOffset = buffer.data.size();
    buffer.data.insert(buffer.data.end(),
                       reinterpret_cast<const unsigned char*>(vertexColorData.data()),
                       reinterpret_cast<const unsigned char*>(vertexColorData.data() + vertexColorData.size()));

    // Add index data to the buffer
    const size_t indexOffset = buffer.data.size();
    buffer.data.insert(buffer.data.end(),
                       reinterpret_cast<const unsigned char*>(indexData.data()),
                       reinterpret_cast<const unsigned char*>(indexData.data() + indexData.size()));

    model.buffers.push_back(buffer);

    // Create a bufferView for the vertex data
    tinygltf::BufferView vertexPositionBufferView;
    vertexPositionBufferView.buffer = 0; // Index of the buffer
    vertexPositionBufferView.byteOffset = 0;
    vertexPositionBufferView.byteLength = vertexPositionData.size() * sizeof(float);
    vertexPositionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(vertexPositionBufferView);

    // Create a bufferView for the color data
    tinygltf::BufferView vertexColorBufferView;
    vertexColorBufferView.buffer = 0; // Index of the buffer
    vertexColorBufferView.byteOffset = colorOffset;
    vertexColorBufferView.byteLength = vertexColorData.size() * sizeof(float);
    vertexColorBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(vertexColorBufferView);

    // Create a bufferView for the index data
    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = 0; // Index of the buffer
    indexBufferView.byteOffset = indexOffset;
    indexBufferView.byteLength = indexData.size() * sizeof(uint16_t);
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    model.bufferViews.push_back(indexBufferView);

    // Create an accessor for the indices
    tinygltf::Accessor indexAccessor;
    indexAccessor.bufferView = 2; // Index of the index bufferView
    indexAccessor.byteOffset = 0;
    indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    indexAccessor.count = static_cast<uint32_t>(indexData.size());
    indexAccessor.type = TINYGLTF_TYPE_SCALAR;
    model.accessors.push_back(indexAccessor);

    // Create an accessor for the vertex positions
    tinygltf::Accessor vertexPositionAccessor;
    vertexPositionAccessor.bufferView = 0; // Index of the vertex bufferView
    vertexPositionAccessor.byteOffset = 0;
    vertexPositionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    vertexPositionAccessor.count = static_cast<uint32_t>(vertexPositionData.size() / 3);
    vertexPositionAccessor.type = TINYGLTF_TYPE_VEC3;
    vertexPositionAccessor.normalized = false;
    model.accessors.push_back(vertexPositionAccessor);

    // Create an accessor for the vertex colors
    tinygltf::Accessor vertexColorAccessor;
    vertexColorAccessor.bufferView = 1; // Index of the vertex color bufferView
    vertexColorAccessor.byteOffset = 0;
    vertexColorAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    vertexColorAccessor.count = static_cast<uint32_t>(vertexColorData.size() / 4);
    vertexColorAccessor.type = TINYGLTF_TYPE_VEC4;
    vertexColorAccessor.normalized = false;
    model.accessors.push_back(vertexColorAccessor);

    // Create a primitive and mesh
    tinygltf::Primitive primitive;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    primitive.indices = 0;
    primitive.attributes["POSITION"] = 1;
    primitive.attributes["COLOR_0"] = 2;
    primitive.material = 0;

    tinygltf::Mesh mesh;
    mesh.primitives.push_back(primitive);

    model.meshes.push_back(mesh);

    // Add the mesh to a node
    tinygltf::Node node;
    node.mesh = 0; // First mesh
    model.nodes.push_back(node);

    scene.nodes.push_back(0); // Add the node to the scene
    model.scenes.push_back(scene);
    model.defaultScene = 0; // Default scene index

    tinygltf::Material material;
    material.name = "Dome_Mat";
    material.doubleSided = true;
    material.pbrMetallicRoughness.metallicFactor = 0.0f;
    material.pbrMetallicRoughness.roughnessFactor = 0.9f;
    model.materials.push_back(material);

    // Save the model
    tinygltf::TinyGLTF gltf;
    std::string err, warn;
    std::string outputFileName = "dome.gltf";
    if (!gltf.WriteGltfSceneToFile(&model, outputFileName, false, true, true, false))
    {
        throw std::runtime_error("Failed to write GLTF: " + err);
    }
}

} // namespace WingsOfSteel::Dome