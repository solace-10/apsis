#include "render/pass/base_render_pass.hpp"

#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "scene/scene.hpp"
#include "scene/systems/landscape_render_system.hpp"
#include "scene/systems/model_render_system.hpp"
#include "scene/systems/sea_render_system.hpp"
#include "pandora.hpp"

namespace WingsOfSteel
{

BaseRenderPass::BaseRenderPass()
: Pass("Base render pass")
{
}

void BaseRenderPass::Execute(wgpu::CommandEncoder& encoder)
{
    wgpu::RenderPassColorAttachment colorAttachment{
        .view = GetWindow()->GetMsaaColorTexture().GetTextureView(),
        .resolveTarget = GetWindow()->GetSceneColorTexture().GetTextureView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }
    };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{
        .view = GetWindow()->GetDepthTexture().GetTextureView(),
        .depthLoadOp = wgpu::LoadOp::Clear,
        .depthStoreOp = wgpu::StoreOp::Store,
        .depthClearValue = 1.0f
    };

    wgpu::RenderPassDescriptor renderpass{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderpass);
    GetRenderSystem()->UpdateGlobalUniforms(renderPass);

    Scene* pScene = GetActiveScene();
    if (pScene)
    {
        LandscapeRenderSystem* pLandscapeRenderSystem = pScene->GetSystem<LandscapeRenderSystem>();
        if (pLandscapeRenderSystem)
        {
            pLandscapeRenderSystem->Render(renderPass);
        }

        ModelRenderSystem* pModelRenderSystem = pScene->GetSystem<ModelRenderSystem>();
        if (pModelRenderSystem)
        {
            pModelRenderSystem->Render(renderPass);
        }

        // The sea is translucent, so it must be drawn after all opaque geometry.
        SeaRenderSystem* pSeaRenderSystem = pScene->GetSystem<SeaRenderSystem>();
        if (pSeaRenderSystem)
        {
            pSeaRenderSystem->Render(renderPass);
        }
    }

    renderPass.End();
}

} // namespace WingsOfSteel