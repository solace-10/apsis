#pragma once

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>

#include "geometry_types.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(GeometryProcessor);
class GeometryProcessor
{
public:
    GeometryProcessor() {}
    ~GeometryProcessor() {}

    std::vector<Vertex> GetUniqueVertices(const std::vector<Vertex>& vertices, float threshold) const;
    std::vector<IndexedTriangle> GetTriangles(const std::vector<Vertex>& vertices) const;
    void Export(const glm::vec2& sourceTextureSize, const std::vector<Vertex>& vertices, const std::vector<IndexedTriangle>& triangles) const;

private:
    void CalculatePositionBounds(const std::vector<Vertex>& vertices, glm::vec3& minBound, glm::vec3& maxBound) const;
    void CalculateColorBounds(const std::vector<Vertex>& vertices, glm::vec3& minBound, glm::vec3& maxBound) const;
    std::vector<Vertex> TransformVertices(const glm::vec2& sourceTextureSize, const std::vector<Vertex>& vertices) const;
};

} // namespace WingsOfSteel