#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "imgui/imgui.hpp"
#include "ui/element.hpp"

namespace WingsOfSteel::UI
{

nlohmann::json Element::Serialize() const
{
    nlohmann::json data;
    data["name"] = m_Name;
    data["type"] = magic_enum::enum_name(GetType());

    if (!HasFlag(Flags::AutoSize))
    {
        data["width"] = m_Size.x;
        data["height"] = m_Size.y;
    }

    return data;
}

void Element::Deserialize(const nlohmann::json& data)
{
    TryDeserialize(data, "name", m_Name, "element");
    TryDeserialize(data, "width", m_Size.x, 0);
    TryDeserialize(data, "height", m_Size.y, 0);
}

void Element::RenderProperties()
{
    ImGui::BeginDisabled(HasFlag(Flags::AutoSize));
    int size[2] = {m_Size.x, m_Size.y};
    if (ImGui::InputInt2("Size", size))
    {
        m_Size = glm::ivec2(size[0], size[1]);
    }
    ImGui::EndDisabled();
}

void Element::BindProperty(const std::string& name, BaseProperty& property)
{
    m_Properties.Add(name, &property);
}

bool Element::TryDeserialize(const nlohmann::json& data, const std::string& key, std::string& value, const std::string& defaultValue)
{
    if (data.contains(key))
    {
        auto& untypedValue = data[key];
        if (untypedValue.is_string())
        {
            value = untypedValue.get<std::string>();
            return true;
        }
    }

    value = defaultValue;
    return false;
}

bool Element::TryDeserialize(const nlohmann::json& data, const std::string& key, int& value, int defaultValue)
{
    if (data.contains(key))
    {
        auto& untypedValue = data[key];
        if (untypedValue.is_number())
        {
            value = untypedValue.get<int>();
            return true;
        }
    }

    value = defaultValue;
    return false;
}

bool Element::TryDeserialize(const nlohmann::json& data, const std::string& key, bool& value, bool defaultValue)
{
    if (data.contains(key))
    {
        auto& untypedValue = data[key];
        if (untypedValue.is_boolean())
        {
            value = untypedValue.get<bool>();
            return true;
        }
    }

    value = defaultValue;
    return false;
}

} // namespace WingsOfSteel::UI