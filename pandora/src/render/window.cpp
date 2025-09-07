#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <GLFW/glfw3.h>

#include <magic_enum.hpp>

#include "render/window.hpp"

#include "core/log.hpp"
#include "imgui_impl_wgpu.h"
#include "pandora.hpp"
#include "render/rendersystem.hpp"

namespace WingsOfSteel::Pandora
{

void OnWindowResizedCallback(GLFWwindow* pWindow, int width, int height)
{
    GetWindow()->OnWindowResized(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

Window::Window(const WindowSettings& windowSettings)
    : m_pWindow(nullptr)
{
    assert(windowSettings.GetMode() != WindowMode::Fullscreen); // Not implemented yet.
    windowSettings.GetSize(m_Width, m_Height);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_pWindow = glfwCreateWindow(m_Width, m_Height, windowSettings.GetTitle().c_str(), nullptr, nullptr);
    glfwSetWindowSizeCallback(m_pWindow, &OnWindowResizedCallback);
    ConfigureSurface();
}

Window::~Window()
{
    glfwDestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}

void Window::OnWindowResized(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

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

        wgpu::SurfaceDescriptor surfaceDesc{ .nextInChain = &canvasDesc };

        m_Surface = GetRenderSystem()->GetInstance().CreateSurface(&surfaceDesc);
#endif

        wgpu::SurfaceCapabilities capabilities;
        m_Surface.GetCapabilities(GetRenderSystem()->GetAdapter(), &capabilities);
        m_Format = capabilities.formats[0];

        Log::Info() << "Configuring surface: ";
        Log::Info() << "- Format: " << magic_enum::enum_name(m_Format);
        Log::Info() << "- Resolution: " << m_Width << "x" << m_Height;
        Log::Info() << "- VSync: " << (m_VSyncEnabled ? "enabled" : "disabled");
    }

    wgpu::PresentMode presentMode = m_VSyncEnabled ? wgpu::PresentMode::Fifo : wgpu::PresentMode::Immediate;
    
    wgpu::SurfaceConfiguration config{
        .device = GetRenderSystem()->GetDevice(),
        .format = m_Format,
        .width = m_Width,
        .height = m_Height,
        .presentMode = presentMode,
    };
    m_Surface.Configure(&config);

    m_DepthTexture = DepthTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, std::string("Window depth buffer"));
    m_MsaaColorTexture = ColorTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, m_Format, RenderSystem::MsaaSampleCount, std::string("Window MSAA color buffer"));
}

void Window::SetVSyncEnabled(bool enabled)
{
    if (m_VSyncEnabled != enabled)
    {
        m_VSyncEnabled = enabled;
        ConfigureSurface(); // Reconfigure surface with new present mode
    }
}

bool Window::IsVSyncEnabled() const
{
    return m_VSyncEnabled;
}

} // namespace WingsOfSteel::Pandora