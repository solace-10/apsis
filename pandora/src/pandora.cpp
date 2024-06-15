#include <iostream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#if defined(TARGET_PLATFORM_NATIVE)
#include <webgpu/webgpu_glfw.h>
#elif defined(TARGET_PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "core/log.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
wgpu::Surface surface;
wgpu::TextureFormat format;
wgpu::RenderPipeline pipeline;

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
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&shaderModuleDescriptor);

    wgpu::ColorTargetState colorTargetState{.format = format};

    wgpu::FragmentState fragmentState{.module = shaderModule,
                                    .targetCount = 1,
                                    .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor descriptor{
        .vertex = {.module = shaderModule},
        .fragment = &fragmentState};
    pipeline = device.CreateRenderPipeline(&descriptor);
}

void GetAdapter(void (*callback)(wgpu::Adapter)) 
{
  instance.RequestAdapter(
      nullptr,
      [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
         const char* message, void* userdata) {
        if (status != WGPURequestAdapterStatus_Success) {
          exit(0);
        }
        wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
        reinterpret_cast<void (*)(wgpu::Adapter)>(userdata)(adapter);
  }, reinterpret_cast<void*>(callback));
}

void GetDevice(void (*callback)(wgpu::Device)) 
{
  adapter.RequestDevice(
      nullptr,
      [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
          const char* message, void* userdata) {
        wgpu::Device device = wgpu::Device::Acquire(cDevice);
        device.SetUncapturedErrorCallback(
            [](WGPUErrorType type, const char* message, void* userdata) {
              std::cout << "Error: " << type << " - message: " << message;
            },
            nullptr);
        reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
  }, reinterpret_cast<void*>(callback));
}

void ConfigureSurface() 
{
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    format = capabilities.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = device,
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

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
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
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#elif defined(TARGET_PLATFORM_WEB)
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &canvasDesc};
    surface = instance.CreateSurface(&surfaceDesc);
#endif

    InitGraphics();

#if defined(TARGET_PLATFORM_NATIVE)
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        Render();
        surface.Present();
        instance.ProcessEvents();
    }
#elif defined(TARGET_PLATFORM_WEB)
    emscripten_set_main_loop(Render, 0, false);
#endif
}

void InitializeLogging();

void Initialize()
{
    InitializeLogging();

    instance = wgpu::CreateInstance();
    GetAdapter([](wgpu::Adapter a) {
        adapter = a;
        GetDevice([](wgpu::Device d) {
            device = d;
            Start();
        });
    });
}

void Run()
{
    
}

void Shutdown()
{

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