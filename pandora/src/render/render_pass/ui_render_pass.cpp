#include "render/render_pass/ui_render_pass.hpp"

#include "imgui/imgui_system.hpp"
#include "render/debug_render.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"

#include "pandora.hpp"

namespace WingsOfSteel
{

UIRenderPass::UIRenderPass()
: RenderPass("UI render pass")
{
}

void UIRenderPass::Render(wgpu::CommandEncoder& encoder)
{
    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Load,
        .storeOp = wgpu::StoreOp::Store
    };

    wgpu::RenderPassDescriptor renderpass{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };

    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderpass);
    GetRenderSystem()->UpdateGlobalUniforms(renderPass);

    GetDebugRender()->Render(renderPass);
    GetImGuiSystem()->Render(renderPass);

    renderPass.End();
}

} // namespace WingsOfSteel