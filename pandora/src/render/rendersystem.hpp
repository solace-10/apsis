#pragma once

#include <functional>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(DebugRender);
DECLARE_SMART_PTR(ShaderCompiler);
DECLARE_SMART_PTR(ShaderEditor);

using OnRenderSystemInitializedCallback = std::function<void()>;

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void Initialize(OnRenderSystemInitializedCallback onInitializedCallback);
    void Update();

    wgpu::Instance& GetInstance() const;
    wgpu::Adapter& GetAdapter() const;
    wgpu::Device& GetDevice() const;

    wgpu::BindGroupLayout& GetGlobalUniformsLayout();

    ShaderCompiler* GetShaderCompiler() const;
    ShaderEditor* GetShaderEditor() const;

private:
    void AcquireDevice(void (*callback)(wgpu::Device));
    void InitializeInternal();

    void CreateGlobalUniforms();
    void UpdateGlobalUniforms(wgpu::RenderPassEncoder& renderPass);

    void RenderBasePass(wgpu::CommandEncoder& encoder);
    void RenderUIPass(wgpu::CommandEncoder& encoder);

    struct GlobalUniforms
    {
        glm::mat4x4 projectionMatrix;
        glm::mat4x4 viewMatrix;
        glm::vec4 cameraPosition;
        float time;
        float windowWidth;
        float windowHeight;
        float _unused[1];
    };
    GlobalUniforms m_GlobalUniforms;

    wgpu::Buffer m_GlobalUniformsBuffer;
    wgpu::BindGroup m_GlobalUniformsBindGroup;
    wgpu::BindGroupLayout m_GlobalUniformsBindGroupLayout;

    ShaderCompilerUniquePtr m_pShaderCompiler;
    ShaderEditorUniquePtr m_pShaderEditor;
};

} // namespace WingsOfSteel::Pandora