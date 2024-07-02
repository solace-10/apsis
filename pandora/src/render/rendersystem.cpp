#include "render/rendersystem.hpp"

#include <cassert>
#include <magic_enum.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/log.hpp"
#include "imgui/imguisystem.hpp"
#include "render/window.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

wgpu::Instance g_Instance = nullptr;
wgpu::Adapter g_Adapter = nullptr;
wgpu::Device g_Device = nullptr;
OnRenderSystemInitializedCallback g_OnRenderSystemInitializedCallback;

void OnRenderSystemInitializedWrapper()
{
    g_OnRenderSystemInitializedCallback();
}

RenderSystem::RenderSystem() 
{
}

RenderSystem::~RenderSystem()
{
    glfwTerminate();
}

void RenderSystem::Initialize(OnRenderSystemInitializedCallback onInitializedCallback)
{
    g_OnRenderSystemInitializedCallback = onInitializedCallback;

    g_Instance = wgpu::CreateInstance();
    if ( g_Instance )
    {
        Log::Info() << "Initialized WebGPU.";
    }
    else
    {
        Log::Error() << "Failed to initialize WebGPU.";
        exit(-1);
    }

    AcquireDevice([](wgpu::Device device) {
        g_Device = device;
        Log::Info() << "WebGPU device acquired.";

        if (!glfwInit()) 
        {
            Log::Error() << "Failed to initialize GLFW.";
            exit(-1);
            return;
        }

        GetRenderSystem()->CreateGlobalUniforms();
        OnRenderSystemInitializedWrapper();
    });
    // Code here will never be reached until shutdown has started.
}

void RenderSystem::Update()
{
#if defined(TARGET_PLATFORM_NATIVE)
    // Tick needs to be called in Dawn to display validation errors
    GetDevice().Tick();
#endif

    wgpu::SurfaceTexture surfaceTexture;
    GetWindow()->GetSurface().GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }
    };

    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                        .colorAttachments = &attachment};

    wgpu::CommandEncoderDescriptor commandEncoderDescriptor{
        .label = "Pandora default command encoder"
    };

    wgpu::CommandEncoder encoder = GetDevice().CreateCommandEncoder(&commandEncoderDescriptor);
    wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderpass);
    UpdateGlobalUniforms(renderPass);
    
    Scene* pActiveScene = GetActiveScene();
    if (pActiveScene)
    {
        pActiveScene->Render(renderPass);
    }

    GetImGuiSystem()->Render(renderPass);

    renderPass.End();

    wgpu::CommandBufferDescriptor commandBufferDescriptor{
        .label = "Pandora default command buffer"
    };
    wgpu::CommandBuffer commands = encoder.Finish(&commandBufferDescriptor);
    GetDevice().GetQueue().Submit(1, &commands);
}

wgpu::Instance RenderSystem::GetInstance() const
{
    return g_Instance;
}

wgpu::Adapter RenderSystem::GetAdapter() const
{
    return g_Adapter;
}

wgpu::Device RenderSystem::GetDevice() const
{
    return g_Device;
}

void RenderSystem::AcquireDevice(void (*callback)(wgpu::Device))
{
    g_Instance.RequestAdapter(
        nullptr,
        // TODO(https://bugs.chromium.org/p/dawn/issues/detail?id=1892): Use
        // wgpu::RequestAdapterStatus, wgpu::Adapter, and wgpu::Device.
        [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
            if (status != WGPURequestAdapterStatus_Success) 
            {
                Log::Error() << "Failed to request adapter, error code " << status;
                exit(-1);
            }

            g_Adapter = wgpu::Adapter::Acquire(adapter);
            Log::Info() << "Adapter successfully requested.";

            wgpu::AdapterProperties properties{};
            g_Adapter.GetProperties(&properties);

            Log::Info() << "Adapter properties:";
            if (properties.vendorName) 
            {
                Log::Info() << " - Vendor name: " << properties.vendorName;
            }
            if (properties.architecture)
            {
                Log::Info() << " - Architecture: " << properties.architecture;
            }
            if (properties.name)
            {
                Log::Info() << " - Name: " << properties.name;
            }
            if (properties.driverDescription)
            {
                Log::Info() << " - Driver: " << properties.driverDescription;
            }
            Log::Info() << " - Backend type: " << magic_enum::enum_name(properties.backendType);

            g_Adapter.RequestDevice(
                nullptr,
                [](WGPURequestDeviceStatus status, WGPUDevice cDevice, const char* message, void* userdata) {
                    wgpu::Device device = wgpu::Device::Acquire(cDevice);
                    device.SetUncapturedErrorCallback(
                            [](WGPUErrorType type, const char* message, void* userdata) {
                                Log::Error() << "Uncaptured device error: " << type << " - message: " << message;
                                exit(-1);
                            },
                            nullptr
                    );
                    reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
                },
                userdata
            );
        },
        reinterpret_cast<void*>(callback));
}

void RenderSystem::CreateGlobalUniforms()
{
    static_assert(sizeof(GlobalUniforms) % 16 == 0);

    using namespace wgpu;

    memset(&m_GlobalUniforms, 0, sizeof(GlobalUniforms));
    m_GlobalUniforms.modelMatrix = glm::mat4x4(1.0f);
    m_GlobalUniforms.projectionMatrix = glm::mat4x4(1.0f);
    m_GlobalUniforms.viewMatrix = glm::mat4x4(1.0f);
    m_GlobalUniforms.time = 0.0f;

    BufferDescriptor bufferDescriptor{
        .label = "Global uniforms buffer",
        .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
        .size = sizeof(GlobalUniforms)
    };

    m_GlobalUniformsBuffer = GetDevice().CreateBuffer(&bufferDescriptor);

    BindGroupLayoutEntry bindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(GlobalUniforms)
        }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry
    };
    m_GlobalUniformsBindGroupLayout = GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    BindGroupEntry bindGroupEntry{
        .binding = 0,
        .buffer = m_GlobalUniformsBuffer,
        .offset = 0,
        .size = sizeof(GlobalUniforms)
    };

    BindGroupDescriptor bindGroupDescriptor{
        .layout = m_GlobalUniformsBindGroupLayout,
        .entryCount = bindGroupLayoutDescriptor.entryCount,
        .entries = &bindGroupEntry
    };

    m_GlobalUniformsBindGroup = GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void RenderSystem::UpdateGlobalUniforms(wgpu::RenderPassEncoder& renderPass)
{
    const float fov = glm::radians(45.0f);
    const float aspectRatio = static_cast<float>(GetWindow()->GetWidth()) / static_cast<float>(GetWindow()->GetHeight());
    const float nearPlane = 1.0f;
    const float farPlane = 200.0f;
    m_GlobalUniforms.projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);

    m_GlobalUniforms.modelMatrix = glm::mat4x4(1.0f);

    if (GetActiveScene() && GetActiveScene()->GetCamera())
    {
        m_GlobalUniforms.viewMatrix = GetActiveScene()->GetCamera()->GetTransform();
    }
    else
    {
        m_GlobalUniforms.viewMatrix = glm::mat4x4(1.0f);
    }

    m_GlobalUniforms.time = static_cast<float>(glfwGetTime());

    GetDevice().GetQueue().WriteBuffer(m_GlobalUniformsBuffer, 0, &m_GlobalUniforms, sizeof(GlobalUniforms));
    renderPass.SetBindGroup(0, m_GlobalUniformsBindGroup);
}

wgpu::BindGroupLayout& RenderSystem::GetGlobalUniformsLayout()
{
    return m_GlobalUniformsBindGroupLayout;
}

} // namespace WingsOfSteel::Pandora