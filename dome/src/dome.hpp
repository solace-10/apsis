#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "point.hpp"
#include "vertex.hpp"

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(BufferedTexture2D);
DECLARE_SMART_PTR(TextureProcessor);

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
    void CalculateEdgePoints();
    void CalculateEdgeVertices();

    TextureProcessorUniquePtr m_pTextureProcessor;
    BufferedTexture2DUniquePtr m_pSourceTexture;

    BufferedTexture2DUniquePtr m_pGreyscaleTexture;
    BufferedTexture2DUniquePtr m_pEdgeTexture;

    bool m_ShowEdgePoints{true};
    int m_EdgePointThreshold{30};
    std::vector<Point> m_EdgePoints;

    bool m_ShowEdgeVertices{true};
    int m_MaxVertexCount{1000};
    float m_Accuracy{1.0f};
    std::vector<Vertex> m_EdgeVertices;
};

} // namespace WingsOfSteel::Dome