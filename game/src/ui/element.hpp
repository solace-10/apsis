#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>
#include <imgui/fonts/icons_font_awesome.hpp>

#include "ui/ui.fwd.hpp"
#include "ui/property.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Element);
class Element : public std::enable_shared_from_this<Element>
{
public:
    Element() {}
    virtual ~Element() {}

    virtual ElementType GetType() const = 0;
    virtual const std::string& GetIcon() const = 0;

    virtual void Render() = 0;
    virtual void RenderProperties();
    virtual nlohmann::json Serialize() const;
    virtual void Deserialize(const nlohmann::json& data);

    void SetName(const std::string& name);
    const std::string& GetName() const;

    void BindProperty(const std::string& name, BaseProperty& property);

protected:
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, std::string& value, const std::string& defaultValue);
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, int& value, int defaultValue);

private:
    PropertyContainer m_Properties;
    std::string m_Name;
};

inline void Element::SetName(const std::string& name)
{
    m_Name = name;
}

inline const std::string& Element::GetName() const
{
    return m_Name;
}

} // namespace WingsOfSteel::TheBrightestStar::UI