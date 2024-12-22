#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "point.hpp"

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

private:
};

} // namespace WingsOfSteel::Dome