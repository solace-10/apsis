#include "geometry_processor.hpp"

#include <delaunator.hpp>

#include "geometry_types.hpp"

namespace WingsOfSteel::Dome
{

std::vector<Vertex> GeometryProcessor::GetUniqueVertices(const std::vector<Vertex>& vertices, float threshold) const
{
    std::vector<Vertex> uniqueVertices;

    // Add first vertex as it's always unique
    if (!vertices.empty()) {
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

} // namespace WingsOfSteel::Dome