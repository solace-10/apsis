#pragma once

#include "render/pass/pass.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(BaseRenderPass);
class BaseRenderPass : public Pass 
{
public:
    BaseRenderPass();
    
    void Execute(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel