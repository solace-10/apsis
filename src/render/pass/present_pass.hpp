#pragma once

#include <vector>

#include "render/pass/pass.hpp"

namespace WingsOfSteel
{

// Blooms, exposes and tonemaps the scene, which is rendered in linear space, encodes it into the
// surface for display and composites the window's overlay on top. Passes after this one draw
// straight onto the surface.
DECLARE_SMART_PTR(PresentPass);
class PresentPass : public Pass
{
public:
    PresentPass();

    void Execute(wgpu::CommandEncoder& encoder) override;

private:
    void CreateRenderPipelines();
    void UpdateBloomTexture();
    void RenderBloom(wgpu::CommandEncoder& encoder);
    void DrawFullscreen(wgpu::CommandEncoder& encoder, const wgpu::RenderPipeline& pipeline, const wgpu::TextureView& target, wgpu::LoadOp loadOp, const wgpu::TextureView& source);

    wgpu::RenderPipeline m_Pipeline;
    wgpu::RenderPipeline m_DownsamplePipeline;
    wgpu::RenderPipeline m_UpsamplePipeline;
    wgpu::Sampler m_Sampler;
    wgpu::Buffer m_SettingsBuffer;

    // Half the window's size, with one view per mip level.
    wgpu::Texture m_BloomTexture;
    std::vector<wgpu::TextureView> m_BloomLevelViews;
};

} // namespace WingsOfSteel
