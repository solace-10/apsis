#include <core/log.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

PrefabData::PrefabData(const std::string& path, OnValueSetCallbackFn callback)
: m_Path(path)
, m_OnValueSetCallback(callback)
{

}

void PrefabData::Initialize()
{
    using namespace Pandora;
    GetResourceSystem()->RequestResource(m_Path, [this](ResourceSharedPtr pResource) {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);

        auto data = pDataStore->Data();
        for (auto it = data.begin(); it != data.end(); ++it)
        {
            const std::string& key = it.key();
            const nlohmann::json& value = it.value();
            if (value.is_number_integer())
            {
                Set(key, value.get<int>());
            }
            else if (value.is_number_float())
            {
                Set(key, value.get<float>());
            }
            else if (value.is_boolean())
            {
                Set(key, value.get<bool>());
            }
            else if (value.is_string())
            {
                Set(key, value.get<std::string>());
            }
            else
            {
                Pandora::Log::Error() << "Unknown type for key: " << key << " in prefab: " << m_Path;
            }
        }
        m_IsLoaded = true;
    });
}

} // namespace WingsOfSteel::TheBrightestStar::UI