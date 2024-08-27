#include <resources/resource_system.hpp>
#include <resources/resource_data_store.hpp>
#include <pandora.hpp>
#include <vfs/vfs.hpp>

#include "items/item_info.hpp"
#include "items/item_manager.hpp"

namespace WingsOfSteel::TheBrightestStar
{

ItemManager::ItemManager()
{
    using namespace Pandora;

    for (const std::string& path : GetVFS()->List("/items/"))
    {
        GetResourceSystem()->RequestResource(path, [this](ResourceSharedPtr pResource) {
            ResourceDataStoreSharedPtr pDataStoreResource = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
            for (auto& entry : pDataStoreResource->Data())
            {
                const std::string name = entry["name"].get<std::string>();
                const std::string displayName = entry["display_name"].get<std::string>();
                const std::string type = entry["type"].get<std::string>();
                m_Database[name] = std::make_unique<ItemInfo>(name, displayName, type);
            }
        });
    }
}
    
const ItemInfo* ItemManager::Get(const std::string& itemName) const
{
    auto it = m_Database.find(itemName);
    if (it == m_Database.end())
    {
        return nullptr;
    }
    else
    {
        return it->second.get();
    }
}

} // namespace WingsOfSteel::TheBrightestStar