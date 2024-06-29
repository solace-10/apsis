#pragma once

#include <functional>

#include <glm/mat4x4.hpp>
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
    void Update();

    wgpu::Instance GetInstance() const;
    wgpu::Adapter GetAdapter() const;
    wgpu::Device GetDevice() const;

private:
    void AcquireDevice(void (*callback)(wgpu::Device));
    void CreateDefaultPipeline();
    void RenderDefaultPipeline();

    void CreateGlobalUniforms();
    void UpdateGlobalUniforms();

    struct GlobalUniforms
    {
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::mat4x4 modelMatrix;
        float time;
        float _unused[3];
    };
    GlobalUniforms m_GlobalUniforms;

    wgpu::RenderPipeline m_DefaultPipeline;
    wgpu::Buffer m_GlobalUniformsBuffer;
};

} // namespace WingsOfSteel::Pandora