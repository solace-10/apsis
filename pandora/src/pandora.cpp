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
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

wgpu::Surface surface;
wgpu::TextureFormat format;
wgpu::RenderPipeline pipeline;

std::unique_ptr<RenderSystem> g_pRenderSystem;

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

    wgpu::ColorTargetState colorTargetState{.format = format};

    wgpu::FragmentState fragmentState{.module = shaderModule,
                                    .targetCount = 1,
                                    .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor descriptor{
        .vertex = {.module = shaderModule},
        .fragment = &fragmentState};
    pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void ConfigureSurface() 
{
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(GetRenderSystem()->GetAdapter(), &capabilities);
    format = capabilities.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = GetRenderSystem()->GetDevice(),
        .format = format,
        .width = kWidth,
        .height = kHeight};
    surface.Configure(&config);
}

void InitGraphics() 
{
    ConfigureSurface();
    CreateRenderPipeline();
}

void Render() 
{
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

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

void Start() 
{
    if (!glfwInit()) 
    {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "The Brightest Star", nullptr, nullptr);

#if defined(TARGET_PLATFORM_NATIVE)
    surface = wgpu::glfw::CreateSurfaceForWindow(GetRenderSystem()->GetInstance(), window);
#elif defined(TARGET_PLATFORM_WEB)
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &canvasDesc};

    surface = GetRenderSystem()->GetInstance().CreateSurface(&surfaceDesc);
#endif

    InitGraphics();

#if defined(TARGET_PLATFORM_NATIVE)
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        Render();
        surface.Present();
        GetRenderSystem()->GetInstance().ProcessEvents();
    }
#elif defined(TARGET_PLATFORM_WEB)
    emscripten_set_main_loop(Render, 0, false);
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

void Shutdown()
{
    g_pRenderSystem.reset();
}

RenderSystem* GetRenderSystem()
{
    return g_pRenderSystem.get();
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