#pragma once

#include "resources/resource.hpp"

#include <webgpu/webgpu_cpp.h>

#include "vfs/vfs.hpp"

namespace WingsOfSteel::Pandora
{

class ResourceTexture2D : public Resource
{
public:
    ResourceTexture2D();
    ~ResourceTexture2D() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);

    wgpu::Texture m_Texture;
    int m_Width;
    int m_Height;
    int m_Channels;
};

} // namespace WingsOfSteel::Pandora