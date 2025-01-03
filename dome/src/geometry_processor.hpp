#pragma once

#include <vector>

#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>

#include "geometry_types.hpp"

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(GeometryProcessor);
class GeometryProcessor
{
public:
    GeometryProcessor() {}
    ~GeometryProcessor() {}

    std::vector<Vertex> GetUniqueVertices(const std::vector<Vertex>& vertices, float threshold) const;
    std::vector<IndexedTriangle> GetTriangles(const std::vector<Vertex>& vertices) const;
    void Export(const std::vector<Vertex>& vertices, const std::vector<IndexedTriangle>& triangles) const;

private:
    void CalculatePositionBounds(const std::vector<Vertex>& vertices, glm::vec3& minBound, glm::vec3& maxBound) const;
    void CalculateColorBounds(const std::vector<Vertex>& vertices, glm::vec3& minBound, glm::vec3& maxBound) const;
};

} // namespace WingsOfSteel::Dome