#include "orbit_overlay_render_pass.hpp"

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <scene/scene.hpp>

#include "systems/orbit_path_render_system.hpp"

namespace WingsOfSteel
{

OrbitOverlayRenderPass::OrbitOverlayRenderPass()
    : Pass("Orbit overlay render pass")
{
}

void OrbitOverlayRenderPass::Execute(wgpu::CommandEncoder& encoder)
{
    wgpu::RenderPassColorAttachment colorAttachment{
        .view = GetWindow()->GetOverlayMsaaColorTexture().GetTextureView(),
        .resolveTarget = GetWindow()->GetOverlayColorTexture().GetTextureView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 0.0 }
    };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{
        .view = GetWindow()->GetDepthTexture().GetTextureView(),
        // Unused by a read-only attachment, but the default is NaN, which browsers reject outright.
        .depthClearValue = 1.0f,
        .depthReadOnly = true
    };

    wgpu::RenderPassDescriptor renderPassDescriptor{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
    GetRenderSystem()->UpdateGlobalUniforms(renderPass);

    Scene* pScene = GetActiveScene();
    if (pScene)
    {
        OrbitPathRenderSystem* pOrbitPathRenderSystem = pScene->GetSystem<OrbitPathRenderSystem>();
        if (pOrbitPathRenderSystem)
        {
            pOrbitPathRenderSystem->Render(renderPass);
        }
    }

    renderPass.End();
}

} // namespace WingsOfSteel
