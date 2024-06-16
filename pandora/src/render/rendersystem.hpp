#pragma once

#include <functional>

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

using OnRenderSystemInitializedCallback = std::function<void()>;

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void Initialize(OnRenderSystemInitializedCallback onInitializedCallback);

    wgpu::Instance GetInstance() const;
    wgpu::Adapter GetAdapter() const;
    wgpu::Device GetDevice() const;

private:
    void AcquireDevice(void (*callback)(wgpu::Device));
};

} // namespace WingsOfSteel::Pandora