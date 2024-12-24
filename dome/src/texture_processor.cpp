#include "texture_processor.hpp"

#include <unordered_map>
#include <string>

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

BufferedTexture2DUniquePtr TextureProcessor::GetEdges(BufferedTexture2D* pSourceTexture) const
{
    // Create the output texture
    BufferedTexture2DUniquePtr pEdgeTexture = std::make_unique<BufferedTexture2D>("edges", pSourceTexture);

    const size_t width = pEdgeTexture->GetWidth();
    const size_t height = pEdgeTexture->GetHeight();
    const size_t channels = pEdgeTexture->GetChannels();
    assert(channels == 4);

    const unsigned char* pSourceData = pSourceTexture->GetTextureData().data();
    unsigned char* pEdgeData = pEdgeTexture->GetTextureData().data();

    // Sobel kernels
    const int sobelX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    const int sobelY[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    auto getPixelIntensity = [&](int x, int y) -> unsigned char {
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height))
        {
            return 0;
        }
        const size_t index = (y * width + x) * channels;
        return pSourceData[index];
    };

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            int gx = 0, gy = 0;

            // Apply Sobel kernels
            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    unsigned char intensity = getPixelIntensity(x + kx, y + ky);
                    gx += sobelX[ky + 1][kx + 1] * intensity;
                    gy += sobelY[ky + 1][kx + 1] * intensity;
                }
            }

            // Compute gradient magnitude
            const unsigned char edgeStrength = static_cast<unsigned char>(std::min(255, static_cast<int>(std::sqrt(gx * gx + gy * gy))));
            const size_t index = (y * width + x) * channels;
            pEdgeData[index] = pEdgeData[index + 1] = pEdgeData[index + 2] = edgeStrength;
        }
    }

    pEdgeTexture->Rebuild();
    return pEdgeTexture;
}

std::vector<Vertex> TextureProcessor::GetVertexFromPoints(const std::vector<Point>& points, int maxPointCount, float accuracy, int width, int height) const
{
    auto addVertex = [](int x, int y, std::unordered_map<std::string, Vertex>& hash) {
        std::ostringstream resultKeyStream;
        resultKeyStream << x << "|" << y;
        std::string resultKey = resultKeyStream.str();

        if (hash.find(resultKey) == hash.end()) 
        {
            hash[resultKey] = {
                .x = x, 
                .y = y,
                .color = {1.0f, 1.0f, 1.0f}
            };
        }
    };

    std::unordered_map<std::string, Vertex> resultHash;

    int gridPointCount = std::max(static_cast<int>(maxPointCount * (1.0f - accuracy)), 5);

    int gridColumns = static_cast<int>(std::round(std::sqrt(gridPointCount)));
    int gridRows = static_cast<int>(std::ceil(static_cast<double>(gridPointCount) / gridColumns));

    int xIncrement = width / gridColumns;
    int yIncrement = height / gridRows;

    int rowIndex = 0;

    for (int y = 0; y < height; y += yIncrement) 
    {
        ++rowIndex;
        int startX = (rowIndex % 2 == 0) ? (xIncrement / 2) : 0;

        for (int x = startX; x < width; x += xIncrement) {
            if (x < width && y < height) {
                addVertex(
                    static_cast<int>(x + std::cos(y) * yIncrement),
                    static_cast<int>(y + std::sin(x) * xIncrement),
                    resultHash
                );
            }
        }
    }

    addVertex(0, 0, resultHash);
    addVertex(width - 1, 0, resultHash);
    addVertex(width - 1, height - 1, resultHash);
    addVertex(0, height - 1, resultHash);

    int remainingPointCount = maxPointCount - static_cast<int>(resultHash.size());
    int edgePointCount = static_cast<int>(points.size());
    int increment = (remainingPointCount > 0) ? (edgePointCount / remainingPointCount) : 0;

    if (maxPointCount > 0 && increment > 0) 
    {
        for (size_t i = 0; i < edgePointCount; i += increment) 
        {
            addVertex(points[i].x, points[i].y, resultHash);
        }
    }

    std::vector<Vertex> result;
    result.reserve(resultHash.size());
    for (const auto& [key, vertex] : resultHash) 
    {
        result.push_back(vertex);
    }

    // Ensure all vertices are within the texture bounds
    for (auto& vertex : result)
    {
        if (vertex.x < 0) vertex.x = 0;
        else if (vertex.x >= width) vertex.x = width - 1;
        if (vertex.y < 0) vertex.y = 0;
        else if (vertex.y >= height) vertex.y = height - 1;
    }

    return result;
}

std::vector<Vertex> TextureProcessor::GetColorizedVertices(const std::vector<Vertex>& vertices, BufferedTexture2D* pSourceTexture) const
{
    std::vector<Vertex> colorizedVertices;

    const size_t channels = pSourceTexture->GetChannels();
    const unsigned char* pData = pSourceTexture->GetTextureData().data();

    for (auto& vertex : vertices)
    {
        assert(vertex.x >= 0);
        assert(vertex.x < pSourceTexture->GetWidth());
        assert(vertex.y >= 0);
        assert(vertex.y < pSourceTexture->GetHeight());

        const size_t index = (vertex.y * pSourceTexture->GetWidth() + vertex.x) * channels;
        assert(index < pSourceTexture->GetTextureData().size());

        colorizedVertices.push_back({
            .x = vertex.x,
            .y = vertex.y,
            .color = {
                static_cast<float>(pData[index]) / 255.0f,
                static_cast<float>(pData[index + 1]) / 255.0f,
                static_cast<float>(pData[index + 2]) / 255.0f
            }
        });
    }

    return colorizedVertices;
}

} // namespace WingsOfSteel::Dome