#pragma once

#include "resources/resource.hpp"

#include <webgpu/webgpu_cpp.h>

#include "vfs/vfs.hpp"

namespace WingsOfSteel::Pandora
{

class ResourceShader : public Resource
{
public:
    ResourceShader();
    ~ResourceShader() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    wgpu::ShaderModule GetShaderModule() const;

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);

    wgpu::ShaderModule m_ShaderModule;
};

} // namespace WingsOfSteel::Pandora