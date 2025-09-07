#pragma once

#include <string>
#include <unordered_map>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ItemInfo);

DECLARE_SMART_PTR(ItemInfoManager);
class ItemInfoManager
{
public:
    ItemInfoManager();
    ~ItemInfoManager() = default;

    const ItemInfoSharedPtr& Get(const std::string& itemName) const;
    ItemInfoSharedPtr Get(const std::string& itemName);

private:
    using ItemInfoDatabase = std::unordered_map<std::string, ItemInfoSharedPtr>;
    ItemInfoDatabase m_Database;
};

} // namespace WingsOfSteel