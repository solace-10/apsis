#pragma once

#include <array>
#include <functional>

#include <imgui.h>
#include <magic_enum.hpp>
#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

using ImGuiGameMenuBarCallback = std::function<void()>;

enum class Font
{
    EXO2_REGULAR_22,
    EXO2_SEMIBOLD_22,
    EXO2_SEMIBOLD_32,
    UBUNTU_MONO_18,
};

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

    ImFont* GetFont(Font font) const;

private:
    void RegisterFonts();
    void RegisterFont(Font font, ImFont* pImFont);
    void ApplyStyle();

    bool m_ShowDemoWindow{ false };
    bool m_DebugDrawDemo{ false };
    bool m_ShowShaderEditor{ false };
    ImGuiGameMenuBarCallback m_GameMenuBarCallback;
    std::array<ImFont*, magic_enum::enum_count<Font>()> m_Fonts;
};

inline ImFont* ImGuiSystem::GetFont(Font font) const
{
    return m_Fonts[static_cast<size_t>(font)];
}

} // namespace WingsOfSteel::Pandora