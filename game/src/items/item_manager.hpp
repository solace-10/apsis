#pragma once

#include <string>
#include <unordered_map>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(ItemInfo);

DECLARE_SMART_PTR(ItemManager);
class ItemManager
{
public:
    ItemManager();
    ~ItemManager() {}

    const ItemInfo* Get(const std::string& itemName) const;

private:
    using ItemDatabase = std::unordered_map<std::string, ItemInfoUniquePtr>;
    ItemDatabase m_Database;
};

} // namespace WingsOfSteel::TheBrightestStar