#include "texture_processor.hpp"

#include "buffered_texture_2d.hpp"
#include <cassert>

namespace WingsOfSteel::Dome
{

std::vector<Point> TextureProcessor::GetEdgePoints(BufferedTexture2D* pSourceTexture, int threshold) const
{
    const int width = pSourceTexture->GetWidth();
    const int height = pSourceTexture->GetHeight();
    const int channels = pSourceTexture->GetChannels();
    TextureData& data = pSourceTexture->GetTextureData();

    const int multiplier = 2;
    std::vector<Point> points;

    for (int y = 0; y < height; y += multiplier) 
    {
        for (int x = 0; x < width; x += multiplier) 
        {
            int sum = 0;
            int total = 0;

            for (int row = -1; row <= 1; row++) 
            {
                int sy = y + row;
                int step = sy * width;

                if (sy >= 0 && sy < height) 
                {
                    for (int col = -1; col <= 1; col++) 
                    {
                        int sx = x + col;

                        if (sx >= 0 && sx < width) 
                        {
                            sum += data[(sx + step) * channels]; // << 2 is equivalent to * 4 for byte addressing
                            total++;
                        }
                    }
                }
            }

            if (total > 0) 
            {
                sum /= total;
            }

            if (sum > threshold) 
            {
                points.push_back({x, y});
            }
        }
    }

    return points;
}
    
BufferedTexture2DUniquePtr TextureProcessor::GetGreyscale(BufferedTexture2D* pSourceTexture) const
{
    BufferedTexture2DUniquePtr pGreyscaleTexture = std::make_unique<BufferedTexture2D>("greyscale", pSourceTexture);

    const size_t size = pGreyscaleTexture->GetTextureData().size();
    const size_t channels = pGreyscaleTexture->GetChannels();
    unsigned char* pData = pGreyscaleTexture->GetTextureData().data();
    assert(channels == 4);
    for (size_t i = 0; i < size; i+= channels)
    {
        const unsigned char brightness = 0.34 * pData[i] + 0.5 * pData[i + 1] + 0.16 * pData[i + 2];
        pData[i] = pData[i + 1] = pData[i + 2] = brightness;
    }
    pGreyscaleTexture->Rebuild();
    return pGreyscaleTexture;
}

} // namespace WingsOfSteel::Dome