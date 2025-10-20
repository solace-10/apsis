#pragma once

#include <list>
#include <functional>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"
#include "render/vertex_types.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(DebugRender);
DECLARE_SMART_PTR(RenderPass);
DECLARE_SMART_PTR(ShaderCompiler);
DECLARE_SMART_PTR(ShaderEditor);
DECLARE_SMART_PTR(VertexBufferSchemas);

using OnRenderSystemInitializedCallback = std::function<void()>;

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void Initialize(OnRenderSystemInitializedCallback onInitializedCallback);
    void Update();

    void AddRenderPass(RenderPassSharedPtr pRenderPass);
    const std::list<RenderPassSharedPtr>& GetRenderPasses() const;
    RenderPassSharedPtr GetRenderPass(const std::string& name) const;
    void ClearRenderPasses();

    wgpu::Instance& GetInstance() const;
    wgpu::Adapter& GetAdapter() const;
    wgpu::Device& GetDevice() const;

    wgpu::BindGroupLayout& GetGlobalUniformsLayout();
    const wgpu::VertexBufferLayout* GetVertexBufferLayout(VertexFormat vertexFormat) const;

    ShaderCompiler* GetShaderCompiler() const;
    ShaderEditor* GetShaderEditor() const;

    void UpdateGlobalUniforms(wgpu::RenderPassEncoder& renderPassEncoder);

    static constexpr size_t MsaaSampleCount = 4;

private:
    void AcquireDevice(void (*callback)(wgpu::Device));
    void InitializeInternal();

    void CreateGlobalUniforms();

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

    std::list<RenderPassSharedPtr> m_RenderPasses;
    VertexBufferSchemasUniquePtr m_pVertexBufferSchemas;
};

} // namespace WingsOfSteel