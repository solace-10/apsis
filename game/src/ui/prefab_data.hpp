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

DECLARE_SMART_PTR(PrefabData);
class PrefabData
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
    

    bool IsLoaded() const { return m_IsLoaded; }

private:
    std::string m_Path;
    std::unordered_map<std::string, PrefabDataValue> m_Data;
    bool m_IsLoaded{false};
    OnValueSetCallbackFn m_OnValueSetCallback;
};

} // namespace WingsOfSteel::TheBrightestStar::UI