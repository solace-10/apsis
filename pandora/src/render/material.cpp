#include <array>

#include "pandora.hpp"
#include "render/material.hpp"
#include "render/rendersystem.hpp"
#include "resources/resource_texture_2d.hpp"

namespace WingsOfSteel
{

Material::Material(const MaterialSpec& materialSpec)
    : m_Spec(materialSpec)
{
    InitializeBindGroupLayout();
    InitializeBlendState();
}

Material::~Material()
{
}

void Material::InitializeBindGroupLayout()
{
    auto CreateDefaultTextureLayoutEntry = [](uint32_t binding) -> wgpu::BindGroupLayoutEntry {
        wgpu::BindGroupLayoutEntry textureLayoutEntry{
            .binding = binding,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e2D }
        };
        return textureLayoutEntry;
    };

    wgpu::SamplerDescriptor samplerDesc{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear
    };

    wgpu::Sampler sampler = GetRenderSystem()->GetDevice().CreateSampler(&samplerDesc);

    wgpu::BindGroupLayoutEntry samplerLayoutEntry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Fragment,
        .sampler{
            .type = wgpu::SamplerBindingType::Filtering }
    };

    std::vector<wgpu::BindGroupLayoutEntry> layoutEntries = {
        samplerLayoutEntry
    };

    std::vector<wgpu::BindGroupEntry> entries = {
        { .binding = 0,
            .sampler = sampler }
    };

    std::array<wgpu::TextureView, 5> textureViews{
        GetBaseColorTexture() ? GetBaseColorTexture()->GetTextureView() : nullptr,
        GetMetallicRoughnessTexture() ? GetMetallicRoughnessTexture()->GetTextureView() : nullptr,
        GetNormalTexture() ? GetNormalTexture()->GetTextureView() : nullptr,
        GetOcclusionTexture() ? GetOcclusionTexture()->GetTextureView() : nullptr,
        GetEmissiveTexture() ? GetEmissiveTexture()->GetTextureView() : nullptr
    };

    for (size_t i = 0; i < textureViews.size(); i++)
    {
        if (textureViews[i] != nullptr)
        {
            const uint32_t binding = static_cast<uint32_t>(i + 1);
            layoutEntries.push_back(CreateDefaultTextureLayoutEntry(binding));
            entries.push_back(
                wgpu::BindGroupEntry{
                    .binding = binding,
                    .textureView = textureViews[i] });
        }
    }

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = layoutEntries.size(),
        .entries = layoutEntries.data()
    };
    m_BindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_BindGroupLayout,
        .entryCount = entries.size(),
        .entries = entries.data()
    };

    m_BindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void Material::InitializeBlendState()
{
    // TODO: If we ever want to support more complex materials we'll need to extend MaterialSpec.
    m_BlendState = wgpu::BlendState{
        .color{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::Zero },
        .alpha{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::Zero }
    };
}

} // namespace WingsOfSteel