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
    : RenderPass("Sector render pass")
{
}

void SectorRenderPass::Render(wgpu::CommandEncoder& encoder)
{
    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = GetWindow()->GetMsaaColorTexture().GetTextureView(),
        .resolveTarget = surfaceTexture.texture.CreateView(),
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
        // The Sun's disc first, as a sky element. The depth buffer has just been cleared, so
        // nothing rejects it; the planet is drawn afterwards and, being opaque, covers the disc
        // wherever the Earth is in front of it, and the atmosphere then blends over what is left
        // so the Sun reddens correctly as it approaches the limb. Its glare goes last - see
        // below.
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

        // Last, over everything including the atmosphere. Glare is scattering in the observer's
        // eye and lens rather than an object out in the scene, so it belongs in front of what it
        // is scattered by - and must not be cut along the planet's silhouette the way the disc
        // itself correctly is.
        if (pSunSystem)
        {
            pSunSystem->RenderGlare(renderPass);
        }
    }

    renderPass.End();
}

} // namespace WingsOfSteel
