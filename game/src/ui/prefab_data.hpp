#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

#include <core/smart_ptr.hpp>
#include <core/log.hpp>
#include <pandora.hpp>

namespace WingsOfSteel::TheBrightestStar::UI
{

using PrefabDataValue = std::variant<std::string, int, float, bool>;
using PrefabDataContainer = std::unordered_map<std::string, PrefabDataValue>;

DECLARE_SMART_PTR(PrefabData);
class PrefabData : public std::enable_shared_from_this<PrefabData>
{
public:
    using OnValueSetCallbackFn = std::function<void(const std::string& key, const PrefabDataValue& value)>;

    PrefabData() {}
    PrefabData(const std::string& path, OnValueSetCallbackFn callback);
    ~PrefabData() {}

    void Initialize();

    inline void Set(const std::string& key, const PrefabDataValue& value)
    {
        m_Data[key] = value;
        m_OnValueSetCallback(key, value);
    }

    template <typename T>
    T Get(const std::string& key) const
    {
        auto it = m_Data.find(key);
        if (it == m_Data.end())
        {
            Pandora::Log::Error() << "Key not found: " << key << " in prefab: " << m_Path;
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
    std::string m_Path;
    PrefabDataContainer m_Data;
    bool m_IsLoaded{false};
    OnValueSetCallbackFn m_OnValueSetCallback;
};

} // namespace WingsOfSteel::TheBrightestStar::UI