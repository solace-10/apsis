#pragma once

#include <nlohmann/json.hpp>

#include "resources/resource.hpp"



namespace WingsOfSteel
{

namespace Json
{
using Data = nlohmann::json;
}

class ResourceDataStore : public Resource
{
public:
    ResourceDataStore();
    ~ResourceDataStore() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    const Json::Data& Data() const;

    void Inject(const Json::Data& data);

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void Save();

    Json::Data m_Data;
};

inline const Json::Data& ResourceDataStore::Data() const
{
    return m_Data;
}

} // namespace WingsOfSteel