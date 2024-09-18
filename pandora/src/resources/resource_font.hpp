#pragma once

#include "resources/resource.hpp"

namespace WingsOfSteel::Pandora
{

class ResourceFont : public Resource
{
public:
    ResourceFont();
    ~ResourceFont() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    const FileData& GetData() const;

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);

    FileSharedPtr m_pUnderlyingFile;
};

} // namespace WingsOfSteel::Pandora