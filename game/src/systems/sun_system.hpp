#pragma once

#include <optional>

#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <resources/resource_shader.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

// Puts the scene's directional light where the Sun actually is, and draws it.
//
// The two halves are one system on purpose: the disc is drawn from
// GlobalUniforms::directionalLightDirection, the same vector this system writes into the light,
// so there is no second copy of the sun direction that could be set from somewhere else and
// leave the disc and the terminator disagreeing about where the Sun is.
class SunSystem : public System
{
public:
    SunSystem();
    ~SunSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;
    void Render(wgpu::RenderPassEncoder& renderPass);

    // While this is set the light is rewritten every frame, which means the Angle and Pitch
    // sliders in the engine's Lighting window have no lasting effect. Clearing it hands the
    // light back to them.
    bool IsTrackingRealSun() const { return m_TrackRealSun; }
    void SetTrackingRealSun(bool state) { m_TrackRealSun = state; }

private:
    void CreateRenderPipeline();
    void HandleShaderInjection();

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    std::optional<SignalId> m_ShaderInjectionSignalId;
    bool m_TrackRealSun{ true };
};

} // namespace WingsOfSteel
