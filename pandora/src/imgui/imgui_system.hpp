#pragma once

#include <functional>

#include <imgui.h>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

using ImGuiGameMenuBarCallback = std::function<void()>;

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

    void SetGameMenuBarCallback(ImGuiGameMenuBarCallback callback);

private:
    void AddDefaultFont();

    bool m_ShowDemoWindow{ false };
    bool m_DebugDrawDemo{ false };
    bool m_ShowShaderEditor{ false };
    ImGuiGameMenuBarCallback m_GameMenuBarCallback;
    ImFont* m_pDefaultFont{ nullptr };
};

} // namespace WingsOfSteel::Pandora