#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel
{

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
    ~Database() {}

    void GetAllObjects(OnAllObjectsReceivedCallback onAllObjectsReceivedCallback, OnDatabaseErrorCallback onDatabaseErrorCallback);

private:
    std::string m_GetAllObjectsEndpoint;

    struct DatabaseCallbacks
    {
        OnAllObjectsReceivedCallback onAllObjectsReceivedCallback;
        OnDatabaseErrorCallback onDatabaseErrorCallback;  
    };

    uint32_t m_RequestIndex{ 0 };
    std::unordered_map<uint32_t, DatabaseCallbacks> m_Requests;
};

} // namespace WingsOfSteel
