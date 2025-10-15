#pragma once

#include <render/render_pass/render_pass.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(ThreatIndicatorRenderPass);
class ThreatIndicatorRenderPass : public RenderPass 
{
public:
    ThreatIndicatorRenderPass();
    
    void Render(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel