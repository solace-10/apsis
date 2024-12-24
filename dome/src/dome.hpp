#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "geometry_types.hpp"

namespace WingsOfSteel::Dome
{

DECLARE_SMART_PTR(BufferedTexture2D);
DECLARE_SMART_PTR(GeometryProcessor);
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
    void BuildGeometry();

    TextureProcessorUniquePtr m_pTextureProcessor;
    BufferedTexture2DUniquePtr m_pSourceTexture;

    BufferedTexture2DUniquePtr m_pGreyscaleTexture;
    BufferedTexture2DUniquePtr m_pEdgeTexture;

    GeometryProcessorUniquePtr m_pGeometryProcessor;

    bool m_ShowEdgePoints{true};
    int m_EdgePointThreshold{30};
    std::vector<Point> m_EdgePoints;

    bool m_ShowEdgeVertices{true};
    int m_MaxVertexCount{1000};
    float m_Accuracy{0.9};
    std::vector<Vertex> m_EdgeVertices;

    bool m_ShowMesh{true};
    bool m_ShowWireframes{true};
    bool m_CollapseVertices{true};
    float m_CollapseThreshold{16.0f};
    std::vector<Vertex> m_ColorizedVertices;
    std::vector<IndexedTriangle> m_IndexedTriangles;
};

} // namespace WingsOfSteel::Dome