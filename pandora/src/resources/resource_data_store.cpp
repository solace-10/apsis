#include "resources/resource_data_store.hpp"

#include "core/log.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ResourceDataStore::ResourceDataStore()
{
}

ResourceDataStore::~ResourceDataStore()
{
}

void ResourceDataStore::Load(const std::string& path)
{
    Resource::Load(path);

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceDataStore::GetResourceType() const
{
    return ResourceType::DataStore;
}

void ResourceDataStore::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        try
        {
            m_Data = nlohmann::json::parse(pFile->GetData());
            SetState(ResourceState::Loaded);
        } catch (nlohmann::json::parse_error& ex)
        {
            Log::Error() << "Error parsing JSON file '" << pFile->GetPath() << "' at byte " << ex.byte << ".";
            SetState(ResourceState::Error);
        }
    }
    else
    {
        SetState(ResourceState::Error);
    }
}

void ResourceDataStore::Inject(const nlohmann::json& data)
{
    m_Data = data;
    Save();
}

void ResourceDataStore::Save()
{
    const std::string dataDump = m_Data.dump(4);
    std::vector<uint8_t> data(dataDump.begin(), dataDump.end());
    if (GetVFS()->FileWrite(GetPath(), data))
    {
        Log::Info() << "Saved data store '" << GetPath() << "'.";
    }
}

} // namespace WingsOfSteel::Pandora
