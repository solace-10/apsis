#include "resources/resource_font.hpp"

#include "core/log.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ResourceFont::ResourceFont()
{
}

ResourceFont::~ResourceFont()
{
}

void ResourceFont::Load(const std::string& path)
{
    Resource::Load(path);

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceFont::GetResourceType() const
{
    return ResourceType::Font;
}

const FileData& ResourceFont::GetData() const
{
    return m_pUnderlyingFile->GetData();
}

void ResourceFont::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        m_pUnderlyingFile = pFile;
        SetState(ResourceState::Loaded);
    }
    else
    {
        SetState(ResourceState::Error);
    }
}

} // namespace WingsOfSteel::Pandora
