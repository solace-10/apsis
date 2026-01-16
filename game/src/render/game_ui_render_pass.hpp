#pragma once

#include <render/render_pass/render_pass.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(GameUIRenderPass);
class GameUIRenderPass : public RenderPass
{
public:
    GameUIRenderPass();

    void Render(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel
