#pragma once

#include <unordered_map>

#include <webgpu/webgpu_cpp.h>

#include "resources/resource.fwd.hpp"

namespace WingsOfSteel
{

class MipLevelGenerator
{
public:
    MipLevelGenerator() = default;
    ~MipLevelGenerator() = default;

    void GenerateMips(wgpu::Texture texture);
    uint32_t GetMipLevelCount(uint32_t width, uint32_t height) const;

private:
    void GenerateMipsInternal(wgpu::Texture texture);
    wgpu::RenderPipeline GetRenderPipeline(wgpu::TextureFormat textureFormat);
    
    ResourceShaderSharedPtr m_pMipLevelShader;
    std::unordered_map<wgpu::TextureFormat, wgpu::RenderPipeline> m_PipelinesByFormat;  
};

} // namespace WingsOfSteel
