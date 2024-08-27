#pragma once

#include <string>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Item);

DECLARE_SMART_PTR(ItemInfo)
class ItemInfo
{
public:
    ItemInfo(const std::string& name, const std::string& displayName, const std::string& type)
    : m_Name(name)
    , m_DisplayName(displayName)
    , m_Type(type)
    {
    }

    ~ItemInfo() {}

    const std::string& GetName() const;
    const std::string& GetDisplayName() const;
    const std::string& GetType() const;

    ItemSharedPtr Create(int quantity = 1) const;

private:
    std::string m_Name;
    std::string m_DisplayName;
    std::string m_Type;
};

inline const std::string& ItemInfo::GetName() const
{
    return m_Name;
}

inline const std::string& ItemInfo::GetDisplayName() const
{
    return m_DisplayName;
}

inline const std::string& ItemInfo::GetType() const
{
    return m_Type;
}

} // namespace WingsOfSteel::TheBrightestStar