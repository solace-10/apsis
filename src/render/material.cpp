#include <array>

#include "pandora.hpp"
#include "render/material.hpp"
#include "render/rendersystem.hpp"
#include "resources/resource_texture_2d.hpp"

namespace WingsOfSteel
{

Material::Material(const MaterialSpec& materialSpec)
    : m_Spec(materialSpec)
    , m_ParameterDefinitions(materialSpec.shaderParameters)
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

    // Add dynamic uniforms buffer if material has shader parameters
    if (!m_ParameterDefinitions.empty())
    {
        // Create storage buffer for dynamic parameters (initial size, will grow as needed)
        wgpu::BufferDescriptor bufferDesc{
            .label = "Dynamic Uniforms Storage Buffer",
            .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
            .size = sizeof(DynamicUniformsData)
        };
        m_DynamicUniformsBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDesc);

        // Next available binding after sampler (0) and textures (1-5)
        const uint32_t dynamicUniformsBinding = 6;

        // Add layout entry for dynamic uniforms storage buffer
        wgpu::BindGroupLayoutEntry dynamicUniformsLayoutEntry{
            .binding = dynamicUniformsBinding,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer{
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = sizeof(DynamicUniformsData) }
        };
        layoutEntries.push_back(dynamicUniformsLayoutEntry);

        // Add bind group entry for dynamic uniforms storage buffer
        wgpu::BindGroupEntry dynamicUniformsEntry{
            .binding = dynamicUniformsBinding,
            .buffer = m_DynamicUniformsBuffer,
            .size = sizeof(DynamicUniformsData)
        };
        entries.push_back(dynamicUniformsEntry);
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
    switch (m_Spec.blendMode)
    {
    case BlendMode::None:
        m_BlendState = wgpu::BlendState{
            .color{
                .srcFactor = wgpu::BlendFactor::One,
                .dstFactor = wgpu::BlendFactor::Zero },
            .alpha{
                .srcFactor = wgpu::BlendFactor::One,
                .dstFactor = wgpu::BlendFactor::Zero }
        };
        break;

    case BlendMode::Blend:
        m_BlendState = wgpu::BlendState{
            .color{
                .srcFactor = wgpu::BlendFactor::SrcAlpha,
                .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha },
            .alpha{
                .srcFactor = wgpu::BlendFactor::One,
                .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha }
        };
        break;

    case BlendMode::Additive:
        m_BlendState = wgpu::BlendState{
            .color{
                .srcFactor = wgpu::BlendFactor::SrcAlpha,
                .dstFactor = wgpu::BlendFactor::One },
            .alpha{
                .srcFactor = wgpu::BlendFactor::One,
                .dstFactor = wgpu::BlendFactor::One }
        };
        break;
    }
}

std::optional<uint32_t> Material::GetParameterOffset(const std::string& name) const
{
    for (const auto& param : m_ParameterDefinitions)
    {
        if (param.name == name)
        {
            return param.offset;
        }
    }
    return std::nullopt;
}

void Material::ResizeDynamicUniformsBuffer(size_t newCapacity)
{
    if (newCapacity <= m_DynamicUniformsBufferCapacity || !HasDynamicUniforms())
    {
        return;
    }

    m_DynamicUniformsBufferCapacity = newCapacity;

    // Recreate buffer with new size
    wgpu::BufferDescriptor bufferDesc{
        .label = "Dynamic Uniforms Storage Buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = newCapacity * sizeof(DynamicUniformsData)
    };
    m_DynamicUniformsBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDesc);

    // Recreate bind group with new buffer (but keep existing layout)
    const uint32_t dynamicUniformsBinding = 6;

    std::vector<wgpu::BindGroupEntry> entries;

    // Add sampler (binding 0)
    wgpu::SamplerDescriptor samplerDesc{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear
    };
    wgpu::Sampler sampler = GetRenderSystem()->GetDevice().CreateSampler(&samplerDesc);
    entries.push_back(wgpu::BindGroupEntry{
        .binding = 0,
        .sampler = sampler
    });

    // Add textures (bindings 1-5)
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
            entries.push_back(wgpu::BindGroupEntry{
                .binding = static_cast<uint32_t>(i + 1),
                .textureView = textureViews[i]
            });
        }
    }

    // Add dynamic uniforms buffer with new size (binding 6)
    entries.push_back(wgpu::BindGroupEntry{
        .binding = dynamicUniformsBinding,
        .buffer = m_DynamicUniformsBuffer,
        .size = newCapacity * sizeof(DynamicUniformsData)
    });

    // Create bind group with existing layout
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_BindGroupLayout,
        .entryCount = entries.size(),
        .entries = entries.data()
    };

    m_BindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

} // namespace WingsOfSteel
