#pragma once

#include <webgpu/webgpu_cpp.h>

#include "render/color_texture.hpp"
#include "render/depth_texture.hpp"
#include "render/window_settings.hpp"

struct GLFWwindow;

namespace WingsOfSteel
{

class Window
{
public:
    Window(const WindowSettings& windowSettings);
    ~Window();

    wgpu::Surface GetSurface() const;
    wgpu::TextureFormat GetSurfaceFormat() const;
    wgpu::TextureFormat GetSceneFormat() const;
    GLFWwindow* GetRawWindow() const;
    const DepthTexture& GetDepthTexture() const;
    DepthTexture& GetDepthTexture();
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    ColorTexture& GetMsaaColorTexture();
    ColorTexture& GetSceneColorTexture();
    ColorTexture& GetOverlayMsaaColorTexture();
    ColorTexture& GetOverlayColorTexture();
    void SetVSyncEnabled(bool enabled);
    bool IsVSyncEnabled() const;

    void OnWindowResized(uint32_t width, uint32_t height);

private:
    void ConfigureSurface();

    GLFWwindow* m_pWindow{ nullptr };
    wgpu::Surface m_Surface;
    wgpu::TextureFormat m_Format;
    uint32_t m_Width{ 0 };
    uint32_t m_Height{ 0 };
    DepthTexture m_DepthTexture;
    ColorTexture m_MsaaColorTexture;
    ColorTexture m_SceneColorTexture;
    ColorTexture m_OverlayMsaaColorTexture;
    ColorTexture m_OverlayColorTexture;
    bool m_VSyncEnabled = true;
    std::string m_Title;
};

inline wgpu::Surface Window::GetSurface() const
{
    return m_Surface;
}

inline wgpu::TextureFormat Window::GetSurfaceFormat() const
{
    return m_Format;
}

// Scene pipelines render here rather than to the surface, so that values above 1.0 survive
// until the present pass encodes them for display.
inline wgpu::TextureFormat Window::GetSceneFormat() const
{
    return wgpu::TextureFormat::RGBA16Float;
}

inline GLFWwindow* Window::GetRawWindow() const
{
    return m_pWindow;
}

inline const DepthTexture& Window::GetDepthTexture() const
{
    return m_DepthTexture;
}

inline DepthTexture& Window::GetDepthTexture()
{
    return m_DepthTexture;
}

inline uint32_t Window::GetWidth() const
{
    return m_Width;
}

inline uint32_t Window::GetHeight() const
{
    return m_Height;
}

inline ColorTexture& Window::GetMsaaColorTexture() { return m_MsaaColorTexture; }
inline ColorTexture& Window::GetSceneColorTexture() { return m_SceneColorTexture; }

// For anything that must be hidden by the scene's depth but kept out of tonemapping. It is in the
// surface format and premultiplied, and PresentPass composites it over the tonemapped scene.
inline ColorTexture& Window::GetOverlayMsaaColorTexture() { return m_OverlayMsaaColorTexture; }
inline ColorTexture& Window::GetOverlayColorTexture() { return m_OverlayColorTexture; }

} // namespace WingsOfSteel
