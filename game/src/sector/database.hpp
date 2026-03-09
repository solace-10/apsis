#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

namespace Private
{
class DatabaseImpl;
class DatabaseNative;
class DatabaseWeb;
}

namespace Json
{
using Data = nlohmann::json;
}

using OnAllObjectsReceivedCallback = std::function<void(const Json::Data& data)>;
using OnDatabaseErrorCallback = std::function<void(const std::string&)>;

DECLARE_SMART_PTR(Database);
class Database
{
public:
    Database();
    ~Database();

    void GetAllObjects(OnAllObjectsReceivedCallback onAllObjectsReceivedCallback, OnDatabaseErrorCallback onDatabaseErrorCallback);

private:
    friend class Private::DatabaseImpl;
    friend class Private::DatabaseNative;
    friend class Private::DatabaseWeb;

    std::string m_GetAllObjectsEndpoint;

    struct DatabaseCallbacks
    {
        OnAllObjectsReceivedCallback onAllObjectsReceivedCallback;
        OnDatabaseErrorCallback onDatabaseErrorCallback;
    };

    uint32_t m_RequestIndex{ 0 };
    std::unordered_map<uint32_t, DatabaseCallbacks> m_Requests;

    std::unique_ptr<Private::DatabaseImpl> m_pImpl;
};

} // namespace WingsOfSteel
