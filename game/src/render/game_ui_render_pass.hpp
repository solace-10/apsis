#pragma once

#include <render/pass/pass.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(GameUIRenderPass);
class GameUIRenderPass : public Pass
{
public:
    GameUIRenderPass();

    void Execute(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel
