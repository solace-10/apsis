#pragma once

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

// The ImGuiSystem handles various implementation details for ImGui, handling
// the integration between the engine and the GLFW/WebGPU backends provided by
// ImGui.
class ImGuiSystem
{
public:
    ImGuiSystem();
    ~ImGuiSystem();

    void OnFrameStart();
    void Update();
    void Render(wgpu::RenderPassEncoder& pass);

private:
};

} // namespace WingsOfSteel::Pandora