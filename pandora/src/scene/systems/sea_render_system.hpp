#pragma once

#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/signal.hpp"
#include "imgui/idebugui.hpp"
#include "resources/resource_shader.hpp"
#include "scene/components/landscape_component.hpp"
#include "scene/components/sea_component.hpp"
#include "scene/systems/system.hpp"

namespace WingsOfSteel
{

class SeaRenderSystem : public System, public IDebugUI
{
public:
    SeaRenderSystem();
    ~SeaRenderSystem();

    void Initialize(Scene* pScene) override{};
    void Update(float delta) override;

    void Render(wgpu::RenderPassEncoder& renderPass);

    void DrawDebugUI() override;

private:
    // Distance to the shoreline, sampled on a grid finer than the landscape's. Built once per
    // landscape generation and discarded as soon as it has been baked into the sea mesh.
    struct ShoreDistanceField
    {
        uint32_t Width{ 0 };
        uint32_t Length{ 0 };
        float CellSize{ 0.0f }; // In meters.
        std::vector<float> Distance; // Meters from the shoreline, 0 at the waterline and growing seawards.

        // Noise evaluated at the nearest point on the shore rather than at the sample itself, so it
        // stays constant along the path a wave travels in on and different beaches can be out of phase
        // without the crests shearing as they move.
        std::vector<float> Phase; // In [0, 1).

        float Sample(const std::vector<float>& values, float worldX, float worldZ) const;
    };

    void GenerateGeometry(const SeaComponent& seaComponent, const LandscapeComponent* pLandscapeComponent);
    ShoreDistanceField BuildShoreDistanceField(const LandscapeComponent* pLandscapeComponent) const;
    void CreateUniforms();
    void UpdateUniforms(const SeaComponent& seaComponent);
    void CreateRenderPipeline();
    void HandleShaderInjection();

    // Mirrored in sea.wgsl; everything is packed into vec4s to sidestep alignment pitfalls.
    struct SeaUniforms
    {
        glm::vec4 waves[3]; // xy: direction, z: steepness, w: wavelength
        glm::vec4 shallowColor;
        glm::vec4 deepColor;
        glm::vec4 params0; // x: depth falloff, y: foam band width, z: foam speed, w: foam reach
        glm::vec4 params1; // x: specular strength, y: fresnel strength, z: sparkle strength, w: sparkle scale
        glm::vec4 params2; // x: amplitude, y: speed, z: foam wash width, w: foam intensity
        glm::vec4 params3; // x: foam warp scale, y: foam warp strength, z: foam crest sharpness
    };

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::Buffer m_VertexBuffer;
    wgpu::Buffer m_IndexBuffer;
    uint32_t m_IndexCount{ 0 };
    uint32_t m_Generation{ 0 };
    uint32_t m_LandscapeGeneration{ 0 };
    SeaUniforms m_Uniforms;
    wgpu::Buffer m_UniformsBuffer;
    wgpu::BindGroupLayout m_UniformsBindGroupLayout;
    wgpu::BindGroup m_UniformsBindGroup;
    bool m_Initialized{ false };
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
