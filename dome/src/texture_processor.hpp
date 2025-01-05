#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "geometry_types.hpp"

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(BufferedTexture2D);

DECLARE_SMART_PTR(TextureProcessor);
class TextureProcessor
{
public:
    TextureProcessor() {}
    ~TextureProcessor() {}

    std::vector<Point> GetEdgePoints(BufferedTexture2D* pSourceTexture, int threshold) const;
    BufferedTexture2DUniquePtr GetGreyscale(BufferedTexture2D* pSourceTexture) const;
    BufferedTexture2DUniquePtr GetEdges(BufferedTexture2D* pSourceTexture) const;
    std::vector<Vertex> GetVerticesFromPoints(const std::vector<Point>& points, int maxPointCount, float accuracy, int width, int height) const;
    std::vector<Vertex> GetColorizedVertices(const std::vector<Vertex>& vertices, BufferedTexture2D* pSourceTexture) const;

private:
};

} // namespace WingsOfSteel::Dome