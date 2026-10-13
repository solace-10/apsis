#include "render/mip_level_generator.hpp"

#include <array>
#include <cmath>

#include "core/log.hpp"
#include "pandora.hpp"
#include "render/rendersystem.hpp"
#include "resources/resource_shader.hpp"
#include "resources/resource_system.hpp"
#include "webgpu/webgpu_cpp.h"

namespace WingsOfSteel
{

void MipLevelGenerator::GenerateMips(wgpu::Texture texture)
{
    if (m_pMipLevelShader)
    {
        GenerateMipsInternal(texture);
    }
    else
    {
        GetResourceSystem()->RequestResource("/shaders/mip_level.wgsl", [this, texture](ResourceSharedPtr pResource) {
            m_pMipLevelShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
            GenerateMipsInternal(texture);
        });
    }
}

void MipLevelGenerator::GenerateMipsInternal(wgpu::Texture texture)
{
    wgpu::SamplerDescriptor samplerDesc{
        .minFilter = wgpu::FilterMode::Linear,
    };

    wgpu::Sampler sampler = GetRenderSystem()->GetDevice().CreateSampler(&samplerDesc);

    wgpu::RenderPipeline pipeline = GetRenderPipeline(texture.GetFormat());

    wgpu::CommandEncoderDescriptor encoderDescriptor{
        .label = "MipLevelGenerator encoder"  
    };
    wgpu::CommandEncoder encoder = GetRenderSystem()->GetDevice().CreateCommandEncoder(&encoderDescriptor);

    const uint32_t mipLevelCount = texture.GetMipLevelCount();
    for (uint32_t mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
    {
        wgpu::TextureViewDescriptor sourceTextureViewDescriptor{
           .baseMipLevel = mipLevel - 1,
           .mipLevelCount = 1 
        };
        
        wgpu::TextureViewDescriptor targetTextureViewDescriptor{
           .baseMipLevel = mipLevel,
           .mipLevelCount = 1 
        };
        
        std::array<wgpu::BindGroupEntry, 2> entries = {
            wgpu::BindGroupEntry{
                .binding = 0,
                .sampler = sampler
                
            },
            wgpu::BindGroupEntry{
                .binding = 1,
                .textureView = texture.CreateView(&sourceTextureViewDescriptor)
            }
        };
        
        wgpu::BindGroupDescriptor bindGroupDescriptor{
            .layout = pipeline.GetBindGroupLayout(0),
            .entryCount = entries.size(),
            .entries = entries.data()
        };

        wgpu::BindGroup bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

        wgpu::RenderPassColorAttachment colorAttachment{
            .view = texture.CreateView(&targetTextureViewDescriptor),
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store  
        };

        wgpu::RenderPassDescriptor renderPassDescriptor{
            .label = "MipLevelGenerator render pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment
        };

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.Draw(6);
        pass.End();
    }

    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    GetRenderSystem()->GetDevice().GetQueue().Submit(1, &commandBuffer);
}

uint32_t MipLevelGenerator::GetMipLevelCount(uint32_t width, uint32_t height) const
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

wgpu::RenderPipeline MipLevelGenerator::GetRenderPipeline(wgpu::TextureFormat textureFormat)
{
    wgpu::RenderPipeline pipeline;
    auto pipelineIt = m_PipelinesByFormat.find(textureFormat);
    if (pipelineIt == m_PipelinesByFormat.end())
    {
        wgpu::ColorTargetState colorTargetState{
            .format = textureFormat
        };
    
        wgpu::FragmentState fragmentState{
            .module = m_pMipLevelShader->GetShaderModule(),
            .targetCount = 1,
            .targets = &colorTargetState
        };
        
        wgpu::RenderPipelineDescriptor descriptor{
            .label = "Mip level generator pipeline",
            .vertex = {
                .module = m_pMipLevelShader->GetShaderModule()
            },
            .fragment = &fragmentState
        };

        pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
        m_PipelinesByFormat[textureFormat] = pipeline;

    }
    else
    {
        pipeline = pipelineIt->second;
    }

    return pipeline;
}

} // namespace WingsOfSteel
