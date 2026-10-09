#include "sector_render_pass.hpp"

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <scene/scene.hpp>
#include <scene/systems/landscape_render_system.hpp>
#include <scene/systems/model_render_system.hpp>

#include "systems/planet_render_system.hpp"
#include "systems/sun_system.hpp"

namespace WingsOfSteel
{

SectorRenderPass::SectorRenderPass()
    : Pass("Sector render pass")
{
}

void SectorRenderPass::Execute(wgpu::CommandEncoder& encoder)
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
        SunSystem* pSunSystem = pScene->GetSystem<SunSystem>();
        if (pSunSystem)
        {
            pSunSystem->RenderDisc(renderPass);
        }

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

        PlanetRenderSystem* pPlanetRenderSystem = pScene->GetSystem<PlanetRenderSystem>();
        if (pPlanetRenderSystem)
        {
            pPlanetRenderSystem->Render(renderPass);
        }

        // Glare is rendered last so it is not occluded by atmosphere or the planet itself.
        if (pSunSystem)
        {
            pSunSystem->RenderGlare(renderPass);
        }
    }

    renderPass.End();
}

} // namespace WingsOfSteel
