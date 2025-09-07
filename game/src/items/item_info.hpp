#pragma once

#include <string>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Item);

DECLARE_SMART_PTR(ItemInfo)
class ItemInfo
{
public:
    enum class Type
    {
        Weapon
    };

    ~ItemInfo() = default;

    static ItemInfoSharedPtr Create();

    virtual Type GetType() const = 0;

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDisplayName() const { return m_DisplayName; }

protected:
    ItemInfo() = default;

private:
    std::string m_Name;
    std::string m_DisplayName;
};

} // namespace WingsOfSteel