#pragma once

#include <render/render_pass/render_pass.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(SectorRenderPass);
class SectorRenderPass : public RenderPass
{
public:
    SectorRenderPass();

    void Render(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel
