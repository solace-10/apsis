#pragma once

#include <imgui/imgui.hpp>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

#include "ui/stackable_element.hpp"
#include "ui/prefab_data.hpp"

namespace WingsOfSteel::UI
{

DECLARE_SMART_PTR(Image);
class Image : public StackableElement
{
public:
    enum class SizeMode
    {
        Source,
        Fixed
    };

    Image() {}
    ~Image() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    void SetSource(const std::string& source);
    void SetSizeMode(SizeMode sizeMode);
    void SetColor(ImU32 color);

private:
    ResourceTexture2DSharedPtr m_pTexture;
    Property<std::string> m_Source;
    SizeMode m_SizeMode{SizeMode::Source};
    ImU32 m_Color{IM_COL32(255, 255, 255, 255)};
};

} // namespace WingsOfSteel::UI