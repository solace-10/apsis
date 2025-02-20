#include <imgui/imgui.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>

#include <pandora.hpp>


#include "ui/image.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

ElementType Image::GetType() const
{
    return ElementType::Image;
}

const std::string& Image::GetIcon() const
{
    static const std::string icon(ICON_FA_IMAGE);
    return icon;
}

void Image::Render()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos() + GetCellPosition();
    const ImVec2 cp1 = cp0 + GetCellSize();

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    if (m_pTexture)
    {
        pDrawList->AddImage(reinterpret_cast<ImTextureID>(m_pTexture->GetTextureView().Get()), cp0, cp1);
    }
    else
    {
        pDrawList->AddRectFilled(cp0, cp1, IM_COL32(255, 0, 0, 40));
        pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
        pDrawList->AddText(cp0 + ImVec2(8.0f, 8.0f), IM_COL32(255, 0, 0, 255), "Image missing");
    }
}

void Image::RenderProperties()
{
    StackableElement::RenderProperties();

    std::string& source = m_Source;
    if (ImGui::InputText("Source", &source))
    {
        SetSource(source);
    }   
}

void Image::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string source;
    TryDeserialize(data, "source", source, "");
    SetSource(source);
}

void Image::SetSource(const std::string& source)
{
    m_Source = source;
    m_pTexture.reset();

    using namespace Pandora;
    if (GetVFS()->Exists(m_Source))
    {
        GetResourceSystem()->RequestResource(m_Source, [this](ResourceSharedPtr pResource)
        {
            if (pResource)
            {
                m_pTexture = std::static_pointer_cast<ResourceTexture2D>(pResource);
            }
        });
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI