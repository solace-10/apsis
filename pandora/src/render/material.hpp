#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"
#include "resources/resource.fwd.hpp"

namespace WingsOfSteel
{

enum class BlendMode
{
    None,
    Blend,
    Additive
};

enum class ShaderParameterType
{
    Float,
    Vec2,
    Vec3,
    Vec4
};

struct ShaderParameterDefinition
{
    std::string name;
    ShaderParameterType type;
    uint32_t offset; // Offset in floats within the params array
    uint32_t componentCount; // 1 for float, 2 for vec2, etc.
    float defaultValue[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
};

struct DynamicUniformsData
{
    static constexpr size_t MaxParams = 8; // 8 vec4s = 32 floats total
    std::array<glm::vec4, MaxParams> params;
};

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
    BlendMode blendMode{ BlendMode::None };
    std::vector<ShaderParameterDefinition> shaderParameters;
};

DECLARE_SMART_PTR(Material);
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
    inline BlendMode GetBlendMode() const { return m_Spec.blendMode; }
    inline const wgpu::BindGroup& GetBindGroup() const { return m_BindGroup; }
    inline const wgpu::BindGroupLayout& GetBindGroupLayout() const { return m_BindGroupLayout; }
    inline const wgpu::BlendState& GetBlendState() const { return m_BlendState; }

    // Dynamic shader parameters
    std::optional<uint32_t> GetParameterOffset(const std::string& name) const;
    inline bool HasDynamicUniforms() const { return !m_ParameterDefinitions.empty(); }
    inline const wgpu::Buffer& GetDynamicUniformsBuffer() const { return m_DynamicUniformsBuffer; }
    inline const std::vector<ShaderParameterDefinition>& GetParameterDefinitions() const { return m_ParameterDefinitions; }
    inline size_t GetDynamicUniformsBufferCapacity() const { return m_DynamicUniformsBufferCapacity; }
    void ResizeDynamicUniformsBuffer(size_t newCapacity);

private:
    void InitializeBindGroupLayout();
    void InitializeBlendState();

    MaterialSpec m_Spec;
    wgpu::BindGroup m_BindGroup;
    wgpu::BindGroupLayout m_BindGroupLayout;
    wgpu::BlendState m_BlendState;

    // Dynamic uniforms (included in group 3)
    std::vector<ShaderParameterDefinition> m_ParameterDefinitions;
    wgpu::Buffer m_DynamicUniformsBuffer;
    size_t m_DynamicUniformsBufferCapacity{ 1 }; // Capacity in number of DynamicUniformsData elements
};

} // namespace WingsOfSteel
