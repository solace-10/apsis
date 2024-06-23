#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

class Window
{
public:
    Window();
    ~Window();

    wgpu::Surface GetSurface() const;
    wgpu::TextureFormat GetTextureFormat() const;
    GLFWwindow* GetRawWindow() const;

private:
    void ConfigureSurface();

    GLFWwindow* m_pWindow;
    wgpu::Surface m_Surface;
    wgpu::TextureFormat m_Format;
    uint32_t m_Width;
    uint32_t m_Height;
};

} // namespace WingsOfSteel::Pandora