#pragma once

#include <string>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

class DepthTexture
{
public:
    DepthTexture() = default;
    DepthTexture(wgpu::Device& device, uint32_t width, uint32_t height, const std::string& label);
    ~DepthTexture() = default;

    inline wgpu::Texture& GetTexture() { return m_Texture; }
    inline wgpu::TextureView& GetTextureView() { return m_TextureView; }
    inline wgpu::Sampler& GetSampler() { return m_Sampler; }

private:
    wgpu::Texture m_Texture;
    wgpu::TextureView m_TextureView;
    wgpu::Sampler m_Sampler;
};

} // namespace WingsOfSteel::Pandora