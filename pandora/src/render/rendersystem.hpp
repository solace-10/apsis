#pragma once

#include <functional>

#include <glm/mat4x4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(DebugRender);

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
    DebugRender* GetDebugRender() const;

    wgpu::BindGroupLayout& GetGlobalUniformsLayout(); 

private:
    void AcquireDevice(void (*callback)(wgpu::Device));

    void CreateGlobalUniforms();
    void UpdateGlobalUniforms(wgpu::RenderPassEncoder& renderPass);
    void CreateDebugRender();

    struct GlobalUniforms
    {
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::mat4x4 modelMatrix;
        float time;
        float _unused[3];
    };
    GlobalUniforms m_GlobalUniforms;

    wgpu::Buffer m_GlobalUniformsBuffer;
    wgpu::BindGroup m_GlobalUniformsBindGroup;
    wgpu::BindGroupLayout m_GlobalUniformsBindGroupLayout;

    DebugRenderUniquePtr m_pDebugRender;
};

} // namespace WingsOfSteel::Pandora