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

private:
    void CreateRenderPipeline();
    void HandleShaderInjection();

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    bool m_Initialized{ false };
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
