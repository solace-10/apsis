#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <magic_enum.hpp>
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
    enum class Flags
    {
        None = 0,
        SelectedInEditor = 1 << 0,
        AutoSize = 1 << 1,
    };

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

    virtual void SetSize(const glm::ivec2& size);
    const glm::ivec2& GetSize() const;

    virtual void SetPosition(const glm::ivec2& position);
    const glm::ivec2& GetPosition() const;

    void BindProperty(const std::string& name, BaseProperty& property);

    void AddFlag(Flags flag);
    void RemoveFlag(Flags flag);
    bool HasFlag(Flags flag) const;

protected:
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, std::string& value, const std::string& defaultValue);
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, int& value, int defaultValue);
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, bool& value, bool defaultValue);

    template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    bool TryDeserialize(const nlohmann::json& data, const std::string& key, T& value, T defaultValue)
    {
        if (data.contains(key))
        {
            value = magic_enum::enum_cast<T>(data[key].get<std::string>()).value_or(defaultValue);
            return true;
        }
        return false;
    }

private:
    PropertyContainer m_Properties;
    std::string m_Name;
    uint32_t m_Flags{0};
    glm::ivec2 m_Size{0};
    glm::ivec2 m_Position{0};
};

inline void Element::SetName(const std::string& name)
{
    m_Name = name;
}

inline const std::string& Element::GetName() const
{
    return m_Name;
}

inline void Element::AddFlag(Flags flag)
{
    m_Flags |= static_cast<uint32_t>(flag);
}

inline void Element::RemoveFlag(Flags flag)
{
    m_Flags &= ~static_cast<uint32_t>(flag);
}

inline bool Element::HasFlag(Flags flag) const
{
    return (m_Flags & static_cast<uint32_t>(flag)) != 0;
}

inline void Element::SetSize(const glm::ivec2& size)
{
    m_Size = size;
}

inline const glm::ivec2& Element::GetSize() const
{
    return m_Size;
}

inline void Element::SetPosition(const glm::ivec2& position)
{
    m_Position = position;
}

inline const glm::ivec2& Element::GetPosition() const
{
    return m_Position;
}

} // namespace WingsOfSteel::TheBrightestStar::UI