#pragma once

#include <optional>

#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/signal.hpp"
#include "resources/resource_shader.hpp"
#include "scene/components/landscape_component.hpp"
#include "scene/systems/system.hpp"

namespace WingsOfSteel
{

class LandscapeRenderSystem : public System
{
public:
    LandscapeRenderSystem();
    ~LandscapeRenderSystem();

    void Initialize(Scene* pScene) override{};
    void Update(float delta) override;

    void Render(wgpu::RenderPassEncoder& renderPass);

private:
    void GenerateGeometry(const LandscapeComponent& landscapeComponent);
    void CreateSplatResources();
    void CreateSplatTexture(uint32_t width, uint32_t length);
    void UpdateSplatTexture(const LandscapeComponent& landscapeComponent);
    void UpdateSplatUniforms(const LandscapeComponent& landscapeComponent);
    void CreateRenderPipeline();
    void HandleShaderInjection();

    // Mirrored in landscape.wgsl; everything is packed into vec4s to sidestep alignment pitfalls.
    struct SplatUniforms
    {
        glm::vec4 colors[LandscapeComponent::kMaxSplatLayers]; // rgb: the color this layer tints the ground with.
        glm::vec4 baseColor; // rgb: what shows through wherever the layers don't add up to 1.
        glm::vec4 params; // xy: the grid's extent in meters, zw: the splat texture's size in texels.
    };
    static_assert(sizeof(SplatUniforms) % 16 == 0);

    // How many array layers the splat texture needs to carry kMaxSplatLayers weights, at four
    // weights to an RGBA texel.
    static constexpr uint32_t sSplatPlaneCount = LandscapeComponent::kMaxSplatLayers / 4;

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::Buffer m_VertexBuffer;
    wgpu::Buffer m_IndexBuffer;
    uint32_t m_VertexCount{ 0 };
    uint32_t m_IndexCount{ 0 };
    uint32_t m_Generation{ 0 };

    wgpu::Buffer m_SplatUniformsBuffer;
    wgpu::Sampler m_SplatSampler;
    wgpu::BindGroupLayout m_SplatBindGroupLayout;
    wgpu::BindGroup m_SplatBindGroup;
    wgpu::Texture m_SplatTexture;
    wgpu::TextureView m_SplatTextureView;
    uint32_t m_SplatWidth{ 0 };
    uint32_t m_SplatLength{ 0 };
    uint32_t m_SplatWeightsGeneration{ 0 };

    bool m_Initialized{ false };
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
