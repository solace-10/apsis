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

class Dome
{
public:
    Dome();
    ~Dome();

    void Initialize();
    void Update(float delta);
    void Shutdown();

private:
    void DrawImGuiMenuBar();

    std::vector<Point> GetEdgePoints(int threshold) const;

    BufferedTexture2DUniquePtr m_pSourceTexture;
    int m_Threshold{220};
    std::vector<Point> m_EdgePoints;
};

} // namespace WingsOfSteel::Dome