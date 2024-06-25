#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <magic_enum.hpp>

#include "render/window.hpp"

#include "core/log.hpp"
#include "imgui_impl_wgpu.h"
#include "render/rendersystem.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

void OnWindowResizedCallback(GLFWwindow* pWindow, int width, int height)
{
    GetWindow()->OnWindowResized(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

Window::Window()
: m_pWindow(nullptr)
{
    m_Width = 1024;
    m_Height = 768;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_pWindow = glfwCreateWindow(m_Width, m_Height, "The Brightest Star", nullptr, nullptr);
    glfwSetWindowSizeCallback(m_pWindow, &OnWindowResizedCallback);
    ConfigureSurface();
}

Window::~Window()
{
    glfwDestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}

wgpu::Surface Window::GetSurface() const
{
    return m_Surface;
}

wgpu::TextureFormat Window::GetTextureFormat() const
{
    return m_Format;
}

GLFWwindow* Window::GetRawWindow() const
{
    return m_pWindow;
}

void Window::OnWindowResized(uint32_t width, uint32_t height)
{
    Log::Info() << "Window resized to " << width << "x" << height;
    m_Width = width;
    m_Height = height;

    ImGui_ImplWGPU_InvalidateDeviceObjects();
    ConfigureSurface();
    ImGui_ImplWGPU_CreateDeviceObjects();
}

void Window::ConfigureSurface()
{
    if (!m_Surface)
    {
#if defined(TARGET_PLATFORM_NATIVE)
        m_Surface = wgpu::glfw::CreateSurfaceForWindow(GetRenderSystem()->GetInstance(), m_pWindow);
#elif defined(TARGET_PLATFORM_WEB)
        wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
        canvasDesc.selector = "#canvas";

        wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &canvasDesc};

        m_Surface = GetRenderSystem()->GetInstance().CreateSurface(&surfaceDesc);
#endif

        wgpu::SurfaceCapabilities capabilities;
        m_Surface.GetCapabilities(GetRenderSystem()->GetAdapter(), &capabilities);
        m_Format = capabilities.formats[0];

        Log::Info() << "Configuring swap chain: ";
        Log::Info() << "- Format: " << magic_enum::enum_name(m_Format);
        Log::Info() << "- Resolution: " << m_Width << "x" << m_Height;
    }

    wgpu::SurfaceConfiguration config{
        .device = GetRenderSystem()->GetDevice(),
        .format = m_Format,
        .width = m_Width,
        .height = m_Height,
    };
    m_Surface.Configure(&config);
}

} // namespace WingsOfSteel::Pandora