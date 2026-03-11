#pragma once

#include <vector>

#include <webgpu/webgpu_cpp.h>

#include <core/color.hpp>
#include <render/vertex_types.hpp>
#include <resources/resource.fwd.hpp>
#include <scene/entity.hpp>
#include <scene/systems/system.hpp>

#include "components/metadata_component.hpp"

namespace WingsOfSteel
{

class OrbitalElementsComponent;

class SpaceObjectRenderSystem : public System
{
public:
    SpaceObjectRenderSystem();
    ~SpaceObjectRenderSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void GenerateLabels();
    void Render(wgpu::RenderPassEncoder& renderPass);
    void NotifyGroupFiltersChanged();

private:
    void CreateRenderPipeline();
    void EnsureLabelsVertexBuffer();
    void GenerateSpaceObjectGroups();
    void GenerateLabelsVertexData();
    bool ShouldDisplayFullLabels() const;
    size_t MakeOrbitalKey(const OrbitalElementsComponent& orbitalElements) const;
    const Color& GetSpaceObjectColor(const MetadataComponent& metadataComponent) const;

    static constexpr size_t kMaxLabels = 65536;
    static constexpr size_t kVerticesPerQuad = 6;
    static constexpr float kQuadHalfSize = 10.0f;

    ResourceShaderSharedPtr m_pShader;
    ResourceBitmapFontSharedPtr m_pFont;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::Buffer m_LabelsVertexBuffer;
    size_t m_LabelsVertexBufferSize{ 0 };
    std::vector<VertexP2C4UV> m_LabelsVertexData;
    wgpu::BindGroupLayout m_TextureBindGroupLayout;
    wgpu::BindGroup m_TextureBindGroup;
    wgpu::Sampler m_Sampler;
    std::vector<std::vector<entt::entity>> m_LabelGroups;
    bool m_LabelsDirty{ false };
};

} // namespace WingsOfSteel
