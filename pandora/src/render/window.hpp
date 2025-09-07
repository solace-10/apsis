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
    wgpu::TextureFormat GetTextureFormat() const;
    GLFWwindow* GetRawWindow() const;
    const DepthTexture& GetDepthTexture() const;
    DepthTexture& GetDepthTexture();
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    ColorTexture& GetMsaaColorTexture();
    void SetVSyncEnabled(bool enabled);
    bool IsVSyncEnabled() const;

    void OnWindowResized(uint32_t width, uint32_t height);

private:
    void ConfigureSurface();

    GLFWwindow* m_pWindow;
    wgpu::Surface m_Surface;
    wgpu::TextureFormat m_Format;
    uint32_t m_Width;
    uint32_t m_Height;
    DepthTexture m_DepthTexture;
    ColorTexture m_MsaaColorTexture;
    bool m_VSyncEnabled = true;
};

inline wgpu::Surface Window::GetSurface() const
{
    return m_Surface;
}

inline wgpu::TextureFormat Window::GetTextureFormat() const
{
    return m_Format;
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

} // namespace WingsOfSteel