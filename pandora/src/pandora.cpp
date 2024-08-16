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
#include "core/random.hpp"
#include "imgui/imgui_system.hpp"
#include "input/input_system.hpp"
#include "render/debug_render.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "scene/scene.hpp"
#include "vfs/vfs.hpp"
#include "vfs/file.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

std::unique_ptr<DebugRender> g_pDebugRender;
std::unique_ptr<ImGuiSystem> g_pImGuiSystem;
std::unique_ptr<InputSystem> g_pInputSystem;
std::unique_ptr<RenderSystem> g_pRenderSystem;
std::unique_ptr<ResourceSystem> g_pResourceSystem;
std::shared_ptr<Scene> g_pActiveScene;
std::unique_ptr<VFS> g_pVFS;
std::unique_ptr<Window> g_pWindow;

GameInitializeCallback g_GameInitializeCallback;
GameUpdateCallback g_GameUpdateCallback;
GameShutdownCallback g_GameShutdownCallback;

float g_PreviousFrameStart = 0.0f;

void InitializeLogging();

void Initialize(GameInitializeCallback gameInitializeCallback, GameUpdateCallback gameUpdateCallback, GameShutdownCallback gameShutdownCallback)
{
    g_GameInitializeCallback = gameInitializeCallback;
    g_GameUpdateCallback = gameUpdateCallback;
    g_GameShutdownCallback = gameShutdownCallback;

    InitializeLogging();

    Random::Initialize();

    g_pVFS = std::make_unique<VFS>();
    g_pVFS->Initialize();

    g_pRenderSystem = std::make_unique<RenderSystem>();
    g_pRenderSystem->Initialize(
        []() -> void 
        {
            g_pResourceSystem = std::make_unique<ResourceSystem>();
            g_pWindow = std::make_unique<Window>();
            g_pInputSystem = std::make_unique<InputSystem>();
            g_pInputSystem->Initialize();
            g_pImGuiSystem = std::make_unique<ImGuiSystem>();
            g_pDebugRender = std::make_unique<DebugRender>();
            g_GameInitializeCallback();
            g_PreviousFrameStart = static_cast<float>(glfwGetTime());

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
    const float now = static_cast<float>(glfwGetTime());
    const float delta = now - g_PreviousFrameStart;
    g_PreviousFrameStart = now;

    GetImGuiSystem()->OnFrameStart();
    
    GetVFS()->Update();
    GetResourceSystem()->Update();
    GetDebugRender()->Update(delta);
    GetImGuiSystem()->Update();

    g_GameUpdateCallback(delta);

    Scene* pActiveScene = GetActiveScene();
    if (pActiveScene)
    {
        pActiveScene->Update(delta);
    }

    GetRenderSystem()->Update();
}

void Shutdown()
{
    g_GameShutdownCallback();

    // Although all the systems are unique pointers and will be cleaned up,
    // this ensures they are shut down in a deterministic order.
    g_pResourceSystem.reset();
    g_pInputSystem.reset();
    g_pWindow.reset();
    g_pActiveScene.reset();
    g_pRenderSystem.reset();
    g_pImGuiSystem.reset();
    g_pVFS.reset();
}

DebugRender* GetDebugRender()
{
    return g_pDebugRender.get();
}

ImGuiSystem* GetImGuiSystem()
{
    return g_pImGuiSystem.get();
}

InputSystem* GetInputSystem()
{
    return g_pInputSystem.get();
}

RenderSystem* GetRenderSystem()
{
    return g_pRenderSystem.get();
}

ResourceSystem* GetResourceSystem()
{
    return g_pResourceSystem.get();
}

Scene* GetActiveScene()
{
    return g_pActiveScene.get();
}

void SetActiveScene(SceneSharedPtr pScene)
{
    g_pActiveScene = pScene;
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