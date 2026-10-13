#include "render/color_texture.hpp"

namespace WingsOfSteel
{

ColorTexture::ColorTexture(wgpu::Device& device, uint32_t width, uint32_t height, wgpu::TextureFormat format, uint32_t sampleCount, const std::string& label)
{
    wgpu::TextureDescriptor textureDesc{
        .label = label.c_str(),
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size{
            .width = width,
            .height = height,
            .depthOrArrayLayers = 1 },
        .format = format,
        .sampleCount = sampleCount
    };

    m_Texture = device.CreateTexture(&textureDesc);
    m_TextureView = m_Texture.CreateView();

    wgpu::SamplerDescriptor samplerDesc{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear
    };
    m_Sampler = device.CreateSampler(&samplerDesc);
}

} // namespace WingsOfSteel