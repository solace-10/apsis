#include <iostream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <memory>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include "core/log.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "vfs/vfs.hpp"
#include "vfs/file.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

std::unique_ptr<RenderSystem> g_pRenderSystem;
std::unique_ptr<VFS> g_pVFS;
std::unique_ptr<Window> g_pWindow;

void InitializeImGui();
void InitializeLogging();

void Initialize()
{
    InitializeLogging();

    g_pVFS = std::make_unique<VFS>();
    g_pVFS->Initialize();

    g_pRenderSystem = std::make_unique<RenderSystem>();
    g_pRenderSystem->Initialize(
        []() -> void 
        {
            if (!glfwInit()) 
            {
                Log::Error() << "Failed to initialize GLFW.";
                exit(-1);
                return;
            }

            g_pWindow = std::make_unique<Window>();

            InitializeImGui();

#if defined(TARGET_PLATFORM_NATIVE)
            while (!glfwWindowShouldClose(GetWindow()->GetRawWindow())) 
            {
                glfwPollEvents();
                Pandora::Update();
                GetWindow()->GetSurface().Present();
                GetRenderSystem()->GetInstance().ProcessEvents();
            }
            Shutdown();
#elif defined(TARGET_PLATFORM_WEB)
            emscripten_set_main_loop(Pandora::Update, 0, true);
#endif
        }
    );
}

void Update()
{
    GetVFS()->Update();

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    GetRenderSystem()->Update();
}

void Shutdown()
{
    // Although all the systems are unique pointers and will be cleaned up,
    // this ensures they are shut down in a deterministic order.
    g_pWindow.reset();
    g_pRenderSystem.reset();
    g_pVFS.reset();

    glfwTerminate();
}

RenderSystem* GetRenderSystem()
{
    return g_pRenderSystem.get();
}

VFS* GetVFS()
{
    return g_pVFS.get();
}

Window* GetWindow()
{
    return g_pWindow.get();
}

void InitializeImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(GetWindow()->GetRawWindow(), true);
#if defined(TARGET_PLATFORM_WEB)
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
    ImGui::GetIO().IniFilename = nullptr; // Disable ini files when targetting web, as we don't support writing temporary files.
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = GetRenderSystem()->GetDevice().Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(GetWindow()->GetTextureFormat());
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);
}

void InitializeLogging()
{
    Log::AddLogTarget( std::make_shared<StdOutLogger>() );

#if defined(TARGET_PLATFORM_NATIVE)
    Log::AddLogTarget( std::make_shared<FileLogger>( "log.txt" ) );
#endif

    Log::Info() << "Logging initialized.";
}

} // namespace WingsOfSteel::Pandora