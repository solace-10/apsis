#pragma once

#include "render/render_pass/render_pass.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(BaseRenderPass);
class BaseRenderPass : public RenderPass 
{
public:
    BaseRenderPass();
    
    void Render(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel