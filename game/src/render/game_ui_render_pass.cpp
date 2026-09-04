#include "game_ui_render_pass.hpp"

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>

#include "sector/sector.hpp"
#include "systems/space_object_render_system.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

GameUIRenderPass::GameUIRenderPass()
    : Pass("Game UI render pass")
{
}

void GameUIRenderPass::Execute(wgpu::CommandEncoder& encoder)
{
    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Load,
        .storeOp = wgpu::StoreOp::Store
    };

    wgpu::RenderPassDescriptor renderPassDescriptor{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDescriptor);
    GetRenderSystem()->UpdateGlobalUniforms(renderPass);

    if (Game::Get()->GetSector())
    {
        SpaceObjectRenderSystem* pSpaceObjectRenderSystem = Game::Get()->GetSector()->GetSystem<SpaceObjectRenderSystem>();
        if (pSpaceObjectRenderSystem)
        {
            pSpaceObjectRenderSystem->Render(renderPass);
        }
    }

    renderPass.End();
}

} // namespace WingsOfSteel
