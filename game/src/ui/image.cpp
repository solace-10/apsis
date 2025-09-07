#include <imgui/imgui.hpp>
#include <resources/resource_system.hpp>
#include <vfs/vfs.hpp>
#include <pandora.hpp>

#include "ui/image.hpp"

namespace WingsOfSteel::UI
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
    const ImVec2 cp0 = ImGui::GetCursorScreenPos();
    ImVec2 cp1 = cp0;

    if (m_SizeMode == SizeMode::Source)
    {
        cp1 = cp0 + glm::vec2(m_pTexture->GetWidth(), m_pTexture->GetHeight());
    }
    else
    {
        cp1 = cp0 + GetSize();
    }

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

    if (HasFlag(Flags::SelectedInEditor))
    {
        pDrawList->AddRect(cp0, cp1, IM_COL32(255, 0, 0, 255));
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

    int sizeMode = static_cast<int>(m_SizeMode);
    if (ImGui::Combo("Size mode", &sizeMode, "Source\0Fixed\0"))
    {
        SetSizeMode(static_cast<SizeMode>(sizeMode));
    }
}

nlohmann::json Image::Serialize() const
{
    nlohmann::json data = StackableElement::Serialize();
    data["size_mode"] = magic_enum::enum_name(m_SizeMode);
    data["source"] = m_Source;
    return data;
}

void Image::Deserialize(const nlohmann::json& data)
{
    StackableElement::Deserialize(data);

    std::string source;
    TryDeserialize(data, "source", source, "");
    SetSource(source);

    SizeMode sizeMode;
    TryDeserialize(data, "size_mode", sizeMode, SizeMode::Source);
    SetSizeMode(sizeMode);
}

void Image::SetSource(const std::string& source)
{
    m_Source = source;
    m_pTexture.reset();

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

void Image::SetSizeMode(SizeMode sizeMode)
{
    m_SizeMode = sizeMode;

    if (m_SizeMode == SizeMode::Fixed)
    {
        RemoveFlag(Flags::AutoSize);
    }
    else
    {
        AddFlag(Flags::AutoSize);
    }
}

} // namespace WingsOfSteel::UI