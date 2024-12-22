#pragma once

#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "point.hpp"

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

    TextureProcessorUniquePtr m_pTextureProcessor;
    BufferedTexture2DUniquePtr m_pSourceTexture;

    BufferedTexture2DUniquePtr m_pGreyscaleTexture;

    int m_Threshold{220};
    std::vector<Point> m_EdgePoints;
};

} // namespace WingsOfSteel::Dome