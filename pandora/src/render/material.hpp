#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "resources/resource.fwd.hpp"

namespace WingsOfSteel
{

struct MaterialSpec
{
    glm::vec4 baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor{ 0.0f };
    float roughnessFactor{ 0.0f };
    glm::vec3 emissiveFactor{ 1.0f, 1.0f, 1.0f };
    ResourceTexture2D* pBaseColorTexture{ nullptr };
    ResourceTexture2D* pMetallicRoughnessTexture{ nullptr };
    ResourceTexture2D* pNormalTexture{ nullptr };
    ResourceTexture2D* pOcclusionTexture{ nullptr };
    ResourceTexture2D* pEmissiveTexture{ nullptr };
};

class Material
{
public:
    Material(const MaterialSpec& materialSpec);
    ~Material();

    inline const glm::vec4& GetBaseColorFactor() const { return m_Spec.baseColorFactor; }
    inline float GetMetallicFactor() const { return m_Spec.metallicFactor; }
    inline float GetRoughnessFactor() const { return m_Spec.roughnessFactor; }
    inline const glm::vec3& GetEmissiveFactor() const { return m_Spec.emissiveFactor; }
    inline ResourceTexture2D* GetBaseColorTexture() const { return m_Spec.pBaseColorTexture; }
    inline ResourceTexture2D* GetMetallicRoughnessTexture() const { return m_Spec.pMetallicRoughnessTexture; }
    inline ResourceTexture2D* GetNormalTexture() const { return m_Spec.pNormalTexture; }
    inline ResourceTexture2D* GetOcclusionTexture() const { return m_Spec.pOcclusionTexture; }
    inline ResourceTexture2D* GetEmissiveTexture() const { return m_Spec.pEmissiveTexture; }
    inline const wgpu::BindGroup& GetBindGroup() const { return m_BindGroup; }
    inline const wgpu::BindGroupLayout& GetBindGroupLayout() const { return m_BindGroupLayout; }
    inline const wgpu::BlendState& GetBlendState() const { return m_BlendState; }

private:
    void InitializeBindGroupLayout();
    void InitializeBlendState();

    MaterialSpec m_Spec;
    wgpu::BindGroup m_BindGroup;
    wgpu::BindGroupLayout m_BindGroupLayout;
    wgpu::BlendState m_BlendState;
};

} // namespace WingsOfSteel