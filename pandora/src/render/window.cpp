#include "webgpu/webgpu_cpp.h"
#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <GLFW/glfw3.h>

#include <magic_enum.hpp>

#include "render/window.hpp"

#include "core/log.hpp"
#include "imgui_impl_wgpu.h"
#include "pandora.hpp"
#include "render/rendersystem.hpp"

namespace WingsOfSteel
{

#if defined(TARGET_PLATFORM_NATIVE)

void OnWindowResizedCallback(GLFWwindow* pWindow, int width, int height)
{
    GetWindow()->OnWindowResized(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

#elif defined(TARGET_PLATFORM_WEB)

bool OnWindowResizedCallback(int eventType, const EmscriptenUiEvent* pEvent, void* userData)
{
    const int width = pEvent->windowInnerWidth;
    const int height = pEvent->windowInnerHeight;
    if (width < 1 || height < 1 || width > 8192 || height > 8192)
    {
        Log::Warning() << "Invalid window dimensions: " << width << "x" << height;
        return false;
    }
    else
    {
        GetWindow()->OnWindowResized(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        return true;
    }
}

#endif

Window::Window(const WindowSettings& windowSettings)
    : m_pWindow(nullptr)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Title = windowSettings.GetTitle();

#if defined(TARGET_PLATFORM_NATIVE)

    assert(windowSettings.GetMode() != WindowMode::Fullscreen); // Not implemented yet.
    windowSettings.GetSize(m_Width, m_Height);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_pWindow = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
    glfwSetWindowSizeCallback(m_pWindow, &OnWindowResizedCallback);

#elif defined(TARGET_PLATFORM_WEB)

    // Query actual browser window size on startup.
    double cssWidth, cssHeight;
    emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
    m_Width = static_cast<uint32_t>(cssWidth);
    m_Height = static_cast<uint32_t>(cssHeight);
    m_pWindow = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);

    // glfwSetWindowSizeCallback() does not work: presumably the "window" is never resized, and just exists
    // within the canvas.
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, &OnWindowResizedCallback);

#endif

    ConfigureSurface();
}

Window::~Window()
{
    // The surface wraps the native window handle, so it has to go first.
    m_MsaaColorTexture = ColorTexture();
    m_SceneColorTexture = ColorTexture();
    m_OverlayMsaaColorTexture = ColorTexture();
    m_OverlayColorTexture = ColorTexture();
    m_DepthTexture = DepthTexture();
    m_Surface = nullptr;

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

#if defined(TARGET_PLATFORM_WEB)
    // On Web the underlying canvas has been resized, resize the GLFW window to match.
    glfwSetWindowSize(m_pWindow, m_Width, m_Height);
#endif

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
        wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
        canvasDesc.selector = "#canvas";
        wgpu::SurfaceDescriptor surfaceDesc{ .nextInChain = &canvasDesc };
        m_Surface = GetRenderSystem()->GetInstance().CreateSurface(&surfaceDesc);
#endif

        wgpu::SurfaceCapabilities capabilities;
        m_Surface.GetCapabilities(GetRenderSystem()->GetAdapter(), &capabilities);

        // Not BGRA8UnormSrgb: glfw::CreateSurfaceForWindow always creates a surface in BGRA8Unorm,
        // and PresentPass encodes the scene for display itself.
        m_Format = wgpu::TextureFormat::BGRA8Unorm;
        bool formatAvailable = false;
        for (int index = 0; index < capabilities.formatCount; index++)
        {
            if (capabilities.formats[index] == m_Format)
            {
                formatAvailable = true;
                break;
            }
        }

        if (!formatAvailable)
        {
            Log::Error() << "Failed to create window surface with format " << magic_enum::enum_name(m_Format);
            return;
        }

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
    m_MsaaColorTexture = ColorTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, GetSceneFormat(), RenderSystem::MsaaSampleCount, std::string("Window MSAA color buffer"));
    m_SceneColorTexture = ColorTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, GetSceneFormat(), 1, std::string("Window scene color buffer"));
    m_OverlayMsaaColorTexture = ColorTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, m_Format, RenderSystem::MsaaSampleCount, std::string("Window overlay MSAA color buffer"));
    m_OverlayColorTexture = ColorTexture(GetRenderSystem()->GetDevice(), m_Width, m_Height, m_Format, 1, std::string("Window overlay color buffer"));
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

} // namespace WingsOfSteel
