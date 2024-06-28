#include <iostream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <memory>

#include "core/log.hpp"
#include "imgui/imguisystem.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "vfs/vfs.hpp"
#include "vfs/file.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

std::unique_ptr<ImGuiSystem> g_pImGuiSystem;
std::unique_ptr<RenderSystem> g_pRenderSystem;
std::unique_ptr<ResourceSystem> g_pResourceSystem;
std::unique_ptr<VFS> g_pVFS;
std::unique_ptr<Window> g_pWindow;

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
            g_pResourceSystem = std::make_unique<ResourceSystem>();
            g_pWindow = std::make_unique<Window>();
            g_pImGuiSystem = std::make_unique<ImGuiSystem>();

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
    GetImGuiSystem()->OnFrameStart();
    
    GetVFS()->Update();
    GetResourceSystem()->Update();
    GetImGuiSystem()->Update();
    
    // Scene management goes here

    GetRenderSystem()->Update();
}

void Shutdown()
{
    // Although all the systems are unique pointers and will be cleaned up,
    // this ensures they are shut down in a deterministic order.
    g_pResourceSystem.reset();
    g_pWindow.reset();
    g_pRenderSystem.reset();
    g_pImGuiSystem.reset();
    g_pVFS.reset();
}

ImGuiSystem* GetImGuiSystem()
{
    return g_pImGuiSystem.get();
}

RenderSystem* GetRenderSystem()
{
    return g_pRenderSystem.get();
}

ResourceSystem* GetResourceSystem()
{
    return g_pResourceSystem.get();
}

VFS* GetVFS()
{
    return g_pVFS.get();
}

Window* GetWindow()
{
    return g_pWindow.get();
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