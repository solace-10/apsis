#include <nlohmann/json.hpp>

#include "ui/element.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

nlohmann::json Element::Serialize() const
{
    nlohmann::json data;
    data["name"] = m_Name;
    return data;
}

void Element::Deserialize(const nlohmann::json& data)
{
    TryDeserialize(data, "name", m_Name, "element");
}

void Element::RenderProperties()
{

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

} // namespace WingsOfSteel::TheBrightestStar::UI