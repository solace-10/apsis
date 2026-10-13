#pragma once

#include <functional>
#include <list>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"
#include "render/vertex_types.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(DebugRender);
DECLARE_SMART_PTR(LightingSystem);
DECLARE_SMART_PTR(MipLevelGenerator);
DECLARE_SMART_PTR(Pass);
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

    void AddPass(PassSharedPtr pPass);
    void RemovePass(const PassSharedPtr& pPass);
    const std::list<PassSharedPtr>& GetPasses() const;
    PassSharedPtr GetPass(const std::string& name) const;
    void ClearPasses();

    wgpu::Instance& GetInstance() const;
    wgpu::Adapter& GetAdapter() const;
    wgpu::Device& GetDevice() const;

    bool HasFeature(wgpu::FeatureName feature) const;

    wgpu::BindGroupLayout& GetGlobalUniformsLayout();
    const wgpu::VertexBufferLayout* GetVertexBufferLayout(VertexFormat vertexFormat) const;

    LightingSystem* GetLightingSystem() const;
    MipLevelGenerator* GetMipLevelGenerator() const;
    ShaderCompiler* GetShaderCompiler() const;
    ShaderEditor* GetShaderEditor() const;

    void UpdateGlobalUniforms(wgpu::RenderPassEncoder& renderPassEncoder);

    static constexpr size_t MsaaSampleCount = 4;

private:
    void AcquireDevice(void (*callback)());
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
        float _padding1;
        glm::vec4 directionalLightColor;
        glm::vec4 directionalLightDirection;
        glm::vec4 ambientLightColor;
    };
    GlobalUniforms m_GlobalUniforms;

    wgpu::Buffer m_GlobalUniformsBuffer;
    wgpu::BindGroup m_GlobalUniformsBindGroup;
    wgpu::BindGroupLayout m_GlobalUniformsBindGroupLayout;

    ShaderCompilerUniquePtr m_pShaderCompiler;
    ShaderEditorUniquePtr m_pShaderEditor;

    std::list<PassSharedPtr> m_Passes;
    VertexBufferSchemasUniquePtr m_pVertexBufferSchemas;

    LightingSystemUniquePtr m_pLightingSystem;
    MipLevelGeneratorUniquePtr m_pMipLevelGenerator;
};

} // namespace WingsOfSteel
