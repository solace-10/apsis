#include <sstream>

#include <xxhash.h>

#include <core/log.hpp>
#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "ui/prefab_data.hpp"
#include "ui/prefab_editor.hpp"
#include "game.hpp"

namespace WingsOfSteel::UI
{

PrefabData::PrefabData(const std::string& path, OnValueSetCallbackFn callback)
: m_Path(path)
, m_OnValueSetCallback(callback)
{

}

void PrefabData::Initialize(OnLoadedCallbackFn onLoadedCallback)
{
    using namespace WingsOfSteel;
    GetResourceSystem()->RequestResource(m_Path, [this, onLoadedCallback](ResourceSharedPtr pResource) {
        m_pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        //BuildFromDataStore();
        m_IsLoaded = true;
        //Game::Get()->GetPrefabEditor()->AddPrefabData(shared_from_this());
        if (onLoadedCallback)
        {
            onLoadedCallback(m_pDataStore->Data());
        }
    });
}

void PrefabData::Save()
{
    if (m_pDataStore)
    {
        const nlohmann::json json = ToJson();
        m_pDataStore->Inject(json);
        m_OriginalHash = m_CurrentHash = GetHash(json);
    }
}

void PrefabData::Revert()
{
    BuildFromDataStore();
}

bool PrefabData::WasModified() const
{
    return m_CurrentHash != m_OriginalHash;
}

void PrefabData::BuildFromDataStore()
{
    if (!m_pDataStore)
    {
        Log::Error() << "Data store not loaded for prefab: " << m_Path;
        return;
    }

    const auto& data = m_pDataStore->Data();
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        const std::string& key = it.key();
        const nlohmann::json& value = it.value();
        if (value.is_number_integer())
        {
            SetInternal(key, value.get<int>());
        }
        else if (value.is_number_float())
        {
            SetInternal(key, value.get<float>());
        }
        else if (value.is_boolean())
        {
            SetInternal(key, value.get<bool>());
        }
        else if (value.is_string())
        {
            SetInternal(key, value.get<std::string>());
        }
        else
        {
            Log::Error() << "Unknown type for key: " << key << " in prefab: " << m_Path;
        }
    }

    m_OriginalHash = m_CurrentHash = GetHash(ToJson());
}

const nlohmann::json PrefabData::ToJson() const
{
    nlohmann::json json;
    for (const auto& [key, value] : m_Data)
    {
        if (std::holds_alternative<int>(value))
        {
            json[key] = std::get<int>(value);
        }
        else if (std::holds_alternative<float>(value))
        {
            json[key] = std::get<float>(value);
        }
        else if (std::holds_alternative<bool>(value))
        {
            json[key] = std::get<bool>(value);
        }
        else if (std::holds_alternative<std::string>(value))
        {
            json[key] = std::get<std::string>(value);
        }
    }
    return json;
}

std::string PrefabData::GetHash(const nlohmann::json& json) const
{
    std::stringstream ss;
    const auto dump = json.dump();
    ss << XXH3_64bits(dump.data(), dump.size());
    return ss.str();
}

} // namespace WingsOfSteel::UI