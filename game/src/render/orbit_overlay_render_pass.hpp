#pragma once

#include <render/pass/pass.hpp>

namespace WingsOfSteel
{

// Orbit paths are display colours that have to match their labels, so they are drawn into the
// window's overlay, which skips tonemapping, while still testing against the scene's depth.
DECLARE_SMART_PTR(OrbitOverlayRenderPass);
class OrbitOverlayRenderPass : public Pass
{
public:
    OrbitOverlayRenderPass();

    void Execute(wgpu::CommandEncoder& encoder) override;
};

} // namespace WingsOfSteel
