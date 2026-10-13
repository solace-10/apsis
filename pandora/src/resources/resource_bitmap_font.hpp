#pragma once

#include "core/smart_ptr.hpp"
#include "resources/resource.fwd.hpp"
#include "resources/resource.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec4.hpp>

#include "render/vertex_types.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ResourceBitmapFont);

struct BitmapFontGlyph
{
    uint32_t id{ 0 };
    uint32_t x{ 0 };
    uint32_t y{ 0 };
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    int32_t xoffset{ 0 };
    int32_t yoffset{ 0 };
    int32_t xadvance{ 0 };
    uint32_t page{ 0 };
};

class ResourceBitmapFont : public Resource
{
public:
    ResourceBitmapFont();
    ~ResourceBitmapFont() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    std::vector<VertexP2C4UV> Generate(const std::string& text, const glm::vec4& color = glm::vec4(1.0f)) const;

    ResourceTexture2D* GetTexture() const { return m_pTexture.get(); }
    uint32_t GetLineHeight() const { return m_LineHeight; }

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void ParseFntFile(const std::string& content);
    void OnTextureLoaded(ResourceSharedPtr pResource);

    std::unordered_map<uint32_t, BitmapFontGlyph> m_Glyphs;
    uint32_t m_LineHeight{ 0 };
    uint32_t m_Base{ 0 };
    uint32_t m_ScaleW{ 0 };
    uint32_t m_ScaleH{ 0 };
    std::string m_BasePath;
    ResourceTexture2DSharedPtr m_pTexture;
};

} // namespace WingsOfSteel
