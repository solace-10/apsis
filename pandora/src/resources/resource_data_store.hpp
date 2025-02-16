#pragma once

#include <nlohmann/json.hpp>

#include "resources/resource.hpp"

namespace WingsOfSteel::Pandora
{

class ResourceDataStore : public Resource
{
public:
    ResourceDataStore();
    ~ResourceDataStore() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    const nlohmann::json& Data() const;

    void Inject(const nlohmann::json& data);

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void Save();

    nlohmann::json m_Data;
};

inline const nlohmann::json& ResourceDataStore::Data() const
{
    return m_Data;
}

} // namespace WingsOfSteel::Pandora