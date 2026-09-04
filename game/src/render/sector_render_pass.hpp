#pragma once

#include <render/pass/pass.hpp>

namespace WingsOfSteel
{

DECLARE_SMART_PTR(SectorRenderPass);
class SectorRenderPass : public Pass
{
public:
    SectorRenderPass();

    void Execute(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel
