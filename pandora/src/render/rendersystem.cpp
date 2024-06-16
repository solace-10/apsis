#include "render/rendersystem.hpp"

#include <cassert>
#include <magic_enum.hpp>
#include "core/log.hpp"

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
        OnRenderSystemInitializedWrapper();
    });
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
                                Log::Warning() << "Uncaptured device error: " << type << " - message: " << message;
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

} // namespace WingsOfSteel::Pandora