#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(BufferedTexture2D);

struct Point {
    int x;
    int y;
};

DECLARE_SMART_PTR(TextureProcessor);
class TextureProcessor
{
public:
    TextureProcessor();
    ~TextureProcessor();

    std::vector<Point> GetEdgePoints(BufferedTexture2D* pSourceTexture, int threshold) const;

private:
};

} // namespace WingsOfSteel::Dome