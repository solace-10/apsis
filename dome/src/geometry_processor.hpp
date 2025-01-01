#pragma once

#include <vector>

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
};

} // namespace WingsOfSteel::Dome