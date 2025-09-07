#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>

#include <core/smart_ptr.hpp>
#include <core/log.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_data_store.hpp>
#include <pandora.hpp>

namespace WingsOfSteel::UI
{

using PrefabDataValue = std::variant<std::string, int, float, bool>;
using PrefabDataContainer = std::unordered_map<std::string, PrefabDataValue>;

DECLARE_SMART_PTR(PrefabData);
class PrefabData : public std::enable_shared_from_this<PrefabData>
{
public:
    using OnValueSetCallbackFn = std::function<void(const std::string& key, const PrefabDataValue& value)>;
    using OnLoadedCallbackFn = std::function<void(const nlohmann::json& data)>;

    PrefabData() {}
    PrefabData(const std::string& path, OnValueSetCallbackFn callback);
    ~PrefabData() {}

    void Initialize(OnLoadedCallbackFn onLoadedCallback);

    void Save();
    void Revert();
    bool WasModified() const;

    inline void Set(const std::string& key, const PrefabDataValue& value)
    {
        m_Data[key] = value;
        m_CurrentHash = GetHash(ToJson());
        m_OnValueSetCallback(key, value);
    }

    template <typename T>
    T Get(const std::string& key) const
    {
        auto it = m_Data.find(key);
        if (it == m_Data.end())
        {
            Log::Error() << "Key not found: " << key << " in prefab: " << m_Path;
            return T{};
        }
        return std::get<T>(it->second);
    }
    
    // Only provide read access to the data!
    // Writing the data must be done through the Set() method to ensure the callback is triggered.
    inline const PrefabDataContainer& GetData() const { return m_Data; }

    inline const std::string& GetPath() const { return m_Path; }
    inline bool IsLoaded() const { return m_IsLoaded; }

private:
    inline void SetInternal(const std::string& key, const PrefabDataValue& value)
    {
        m_Data[key] = value;
        m_OnValueSetCallback(key, value);
    }

    void BuildFromDataStore();
    const nlohmann::json ToJson() const;
    std::string GetHash(const nlohmann::json& json) const;

    std::string m_Path;
    PrefabDataContainer m_Data;
    bool m_IsLoaded{false};
    OnValueSetCallbackFn m_OnValueSetCallback;
    std::string m_OriginalHash;
    std::string m_CurrentHash;
    ResourceDataStoreSharedPtr m_pDataStore;
};

} // namespace WingsOfSteel::UI