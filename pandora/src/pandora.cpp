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
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

wgpu::RenderPipeline pipeline;

std::unique_ptr<RenderSystem> g_pRenderSystem;
std::unique_ptr<Window> g_pWindow;

const char shaderCode[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) ->
      @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 0, 1);
    }
)";

void CreateRenderPipeline() 
{
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode;

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
        .nextInChain = &wgslDesc
    };
    wgpu::ShaderModule shaderModule = GetRenderSystem()->GetDevice().CreateShaderModule(&shaderModuleDescriptor);

    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{.module = shaderModule,
                                    .targetCount = 1,
                                    .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor descriptor{
        .vertex = {.module = shaderModule},
        .fragment = &fragmentState};
    pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void Start() 
{
    if (!glfwInit()) 
    {
        return;
    }

    g_pWindow = std::make_unique<Window>();

    CreateRenderPipeline();

#if defined(TARGET_PLATFORM_NATIVE)
    while (!glfwWindowShouldClose(GetWindow()->GetRawWindow())) 
    {
        glfwPollEvents();
        Pandora::Update();
        GetWindow()->GetSurface().Present();
        GetRenderSystem()->GetInstance().ProcessEvents();
    }
#elif defined(TARGET_PLATFORM_WEB)
    emscripten_set_main_loop(Pandora::Update, 0, false);
#endif
}

void InitializeLogging();

void Initialize()
{
    InitializeLogging();

    g_pRenderSystem = std::make_unique<RenderSystem>();
    g_pRenderSystem->Initialize(
        []() -> void {
            Start();
        }
    );
}

void Update()
{
    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store};

    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                        .colorAttachments = &attachment};

    wgpu::CommandEncoder encoder = GetRenderSystem()->GetDevice().CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    GetRenderSystem()->GetDevice().GetQueue().Submit(1, &commands);
}

void Shutdown()
{
    g_pRenderSystem.reset();
}

RenderSystem* GetRenderSystem()
{
    return g_pRenderSystem.get();
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