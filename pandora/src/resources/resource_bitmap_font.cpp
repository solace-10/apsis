#include "resources/resource_bitmap_font.hpp"

#include <sstream>

#include "core/log.hpp"
#include "pandora.hpp"
#include "resources/resource_system.hpp"
#include "resources/resource_texture_2d.hpp"

namespace WingsOfSteel
{

ResourceBitmapFont::ResourceBitmapFont()
{
}

ResourceBitmapFont::~ResourceBitmapFont()
{
}

void ResourceBitmapFont::Load(const std::string& path)
{
    Resource::Load(path);

    // Extract base path for loading the texture relative to the .fnt file
    const size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos)
    {
        m_BasePath = path.substr(0, lastSlash + 1);
    }
    else
    {
        m_BasePath = "/";
    }

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceBitmapFont::GetResourceType() const
{
    return ResourceType::BitmapFont;
}

void ResourceBitmapFont::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result != FileReadResult::Ok)
    {
        SetState(ResourceState::Error);
        return;
    }

    const std::string content(pFile->GetData().begin(), pFile->GetData().end());
    ParseFntFile(content);
}

void ResourceBitmapFont::ParseFntFile(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    std::string texturePath;

    while (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        std::string lineType;
        lineStream >> lineType;

        if (lineType == "common")
        {
            std::string token;
            while (lineStream >> token)
            {
                const size_t equalsPos = token.find('=');
                if (equalsPos == std::string::npos)
                {
                    continue;
                }

                const std::string key = token.substr(0, equalsPos);
                const std::string value = token.substr(equalsPos + 1);

                if (key == "lineHeight")
                {
                    m_LineHeight = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "base")
                {
                    m_Base = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "scaleW")
                {
                    m_ScaleW = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "scaleH")
                {
                    m_ScaleH = static_cast<uint32_t>(std::stoi(value));
                }
            }
        }
        else if (lineType == "page")
        {
            const size_t fileStart = line.find("file=\"");
            if (fileStart != std::string::npos)
            {
                const size_t nameStart = fileStart + 6;
                const size_t nameEnd = line.find('"', nameStart);
                if (nameEnd != std::string::npos)
                {
                    texturePath = m_BasePath + line.substr(nameStart, nameEnd - nameStart);
                }
            }
        }
        else if (lineType == "char")
        {
            BitmapFontGlyph glyph;
            std::string token;
            while (lineStream >> token)
            {
                const size_t equalsPos = token.find('=');
                if (equalsPos == std::string::npos)
                {
                    continue;
                }

                const std::string key = token.substr(0, equalsPos);
                const std::string value = token.substr(equalsPos + 1);

                if (key == "id")
                {
                    glyph.id = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "x")
                {
                    glyph.x = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "y")
                {
                    glyph.y = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "width")
                {
                    glyph.width = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "height")
                {
                    glyph.height = static_cast<uint32_t>(std::stoi(value));
                }
                else if (key == "xoffset")
                {
                    glyph.xoffset = std::stoi(value);
                }
                else if (key == "yoffset")
                {
                    glyph.yoffset = std::stoi(value);
                }
                else if (key == "xadvance")
                {
                    glyph.xadvance = std::stoi(value);
                }
                else if (key == "page")
                {
                    glyph.page = static_cast<uint32_t>(std::stoi(value));
                }
            }
            m_Glyphs[glyph.id] = glyph;
        }
    }

    if (texturePath.empty())
    {
        Log::Error() << "BitmapFont: No texture path found in .fnt file";
        SetState(ResourceState::Error);
        return;
    }

    GetResourceSystem()->RequestResource(texturePath,
        [this](ResourceSharedPtr pResource) {
            this->OnTextureLoaded(pResource);
        });
}

void ResourceBitmapFont::OnTextureLoaded(ResourceSharedPtr pResource)
{
    m_pTexture = std::dynamic_pointer_cast<ResourceTexture2D>(pResource);
    if (m_pTexture)
    {
        SetState(ResourceState::Loaded);
    }
    else
    {
        Log::Error() << "BitmapFont: Failed to load texture";
        SetState(ResourceState::Error);
    }
}

std::vector<VertexP2C4UV> ResourceBitmapFont::Generate(const std::string& text, const glm::vec4& color) const
{    
    std::vector<VertexP2C4UV> vertices;
    if (m_Glyphs.empty())
    {
        Log::Error() << "Trying to generate text for a bitmap font before it has finished loading, or font has no glyphs.";
        return vertices;
    }

    vertices.reserve(text.size() * 6); // 6 vertices per character (2 triangles)

    float cursorX = 0.0f;
    const float cursorY = 0.0f;

    for (const char c : text)
    {
        const auto it = m_Glyphs.find(static_cast<uint32_t>(c));
        if (it == m_Glyphs.end())
        {
            continue;
        }

        const BitmapFontGlyph& glyph = it->second;

        const float x = cursorX + static_cast<float>(glyph.xoffset);
        const float y = cursorY + static_cast<float>(glyph.yoffset);
        const float w = static_cast<float>(glyph.width);
        const float h = static_cast<float>(glyph.height);

        const float u0 = static_cast<float>(glyph.x) / static_cast<float>(m_ScaleW);
        const float v0 = static_cast<float>(glyph.y) / static_cast<float>(m_ScaleH);
        const float u1 = static_cast<float>(glyph.x + glyph.width) / static_cast<float>(m_ScaleW);
        const float v1 = static_cast<float>(glyph.y + glyph.height) / static_cast<float>(m_ScaleH);

        // Triangle 1: top-left, top-right, bottom-right
        vertices.push_back({ { x, y }, color, { u0, v0 } });
        vertices.push_back({ { x + w, y }, color, { u1, v0 } });
        vertices.push_back({ { x + w, y + h }, color, { u1, v1 } });

        // Triangle 2: top-left, bottom-right, bottom-left
        vertices.push_back({ { x, y }, color, { u0, v0 } });
        vertices.push_back({ { x + w, y + h }, color, { u1, v1 } });
        vertices.push_back({ { x, y + h }, color, { u0, v1 } });

        cursorX += static_cast<float>(glyph.xadvance);
    }

    return vertices;
}

} // namespace WingsOfSteel
