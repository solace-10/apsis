#pragma once

#include <optional>

#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <resources/resource_shader.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class PlanetRenderSystem : public System
{
public:
    PlanetRenderSystem();
    ~PlanetRenderSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;
    void Render(wgpu::RenderPassEncoder& renderPass);

    bool IsWireframeEnabled() const { return m_RenderWireframe; }
    void SetWireframeEnabled(bool enabled) { m_RenderWireframe = enabled; }

private:
    void CreateRenderPipeline();
    void CreateWireframePipeline();
    void HandleShaderInjection();

    ResourceShaderSharedPtr m_pShader;
    ResourceShaderSharedPtr m_pWireframeShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::RenderPipeline m_WireframePipeline;
    bool m_Initialized{ false };
    bool m_WireframeInitialized{ false };
    bool m_RenderWireframe{ false };
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
