#pragma once

#include <glm/vec3.hpp>

#include "resources/resource.fwd.hpp"

namespace WingsOfSteel::Pandora
{

struct MaterialSpec
{
    glm::vec3 baseColorFactor{ 1.0f, 1.0f, 1.0f };
    float metallicFactor{ 0.0f };
    float roughnessFactor{ 0.0f };
    ResourceTexture2D* pBaseColorTexture{ nullptr };
    ResourceTexture2D* pMetallicRoughnessTexture{ nullptr };
    ResourceTexture2D* pNormalTexture{ nullptr };
    ResourceTexture2D* pOcclusionTexture{ nullptr };
    glm::vec3 emissiveFactor{ 1.0f, 1.0f, 1.0f };   
};

class Material
{
public:
    Material(const MaterialSpec& materialSpec)
    : m_Spec(materialSpec)
    {
    }

    ~Material()
    {
    }

    inline const glm::vec3& GetBaseColorFactor() const { return m_Spec.baseColorFactor; }
    inline float GetMetallicFactor() const { return m_Spec.metallicFactor; }
    inline float GetRoughnessFactor() const { return m_Spec.roughnessFactor; }
    inline ResourceTexture2D* GetBaseColorTexture() const { return m_Spec.pBaseColorTexture; }
    inline ResourceTexture2D* GetMetallicRoughnessTexture() const { return m_Spec.pMetallicRoughnessTexture; }
    inline ResourceTexture2D* GetNormalTexture() const { return m_Spec.pNormalTexture; }
    inline ResourceTexture2D* GetOcclusionTexture() const { return m_Spec.pOcclusionTexture; }
    inline const glm::vec3& GetEmissiveFactor() const { return m_Spec.emissiveFactor; }

private:
    MaterialSpec m_Spec;
};

} // namespace WingsOfSteel::Pandora