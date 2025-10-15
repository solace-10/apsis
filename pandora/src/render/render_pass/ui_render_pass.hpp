#pragma once

#include "render/render_pass/render_pass.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(UIRenderPass);
class UIRenderPass : public RenderPass 
{
public:
    UIRenderPass();
    
    void Render(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel