#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#include "render/window_settings.hpp"

namespace WingsOfSteel::Pandora
{

class Window
{
public:
    Window(const WindowSettings& windowSettings);
    ~Window();

    wgpu::Surface GetSurface() const;
    wgpu::TextureFormat GetTextureFormat() const;
    GLFWwindow* GetRawWindow() const;
    void OnWindowResized(uint32_t width, uint32_t height);

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

private:
    void ConfigureSurface();

    GLFWwindow* m_pWindow;
    wgpu::Surface m_Surface;
    wgpu::TextureFormat m_Format;
    uint32_t m_Width;
    uint32_t m_Height;
};

} // namespace WingsOfSteel::Pandora