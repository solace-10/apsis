#pragma once

#include "render/pass/pass.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(UIRenderPass);
class UIRenderPass : public Pass 
{
public:
    UIRenderPass();
    
    void Execute(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel