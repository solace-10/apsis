#pragma once

#include "resources/resource.hpp"
#include "resources/resource.fwd.hpp"

#include <atomic>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/signal.hpp"
#include "render/material.hpp"
#include "resources/resource_shader.hpp"

namespace tinygltf
{
    class Accessor;
    class Model;
    class Primitive;
}

namespace WingsOfSteel::Pandora
{

class ResourceModel : public Resource
{
public:
    ResourceModel();
    ~ResourceModel() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    void Render(wgpu::RenderPassEncoder& renderPass, const glm::mat4& transform);

    using NodeIndices = std::vector<uint32_t>;
    class Node
    {
    public:
        Node(const std::string& name, const glm::mat4& transform, const NodeIndices& children, bool isRoot, std::optional<uint32_t> meshId)
        : m_Transform(transform)
        , m_Name(name)
        , m_Children(children)
        , m_IsRoot(isRoot)
        , m_MeshId(meshId)
        {}

        ~Node() {}

        inline const glm::mat4 GetTransform() const { return m_Transform; }
        inline const std::string& GetName() const { return m_Name; }
        inline const NodeIndices& GetChildren() const { return m_Children; }
        inline bool IsRoot() const { return m_IsRoot; }
        inline std::optional<uint32_t> GetMeshId() const { return m_MeshId; }

    private:
        glm::mat4 m_Transform;
        std::string m_Name;
        NodeIndices m_Children;
        bool m_IsRoot;
        std::optional<uint32_t> m_MeshId;
    };

private:
    void InitializeShaderLocationsMap();
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void LoadDependentResources();
    void OnDependentResourcesLoaded();
    void SetupMaterials();
    void SetupNodes();
    void SetupMeshes();
    void SetupPrimitive(uint32_t meshId, tinygltf::Primitive* pPrimitive);
    std::optional<int> GetShaderLocation(const std::string& attributeName) const;
    wgpu::IndexFormat GetIndexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::VertexFormat GetVertexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::PrimitiveTopology GetPrimitiveTopology(const tinygltf::Primitive* pPrimitive) const;
    int GetNumberOfComponentsForType(int type) const;
    ResourceShader* GetShaderForPrimitive(tinygltf::Primitive* pPrimitive) const;
    void CreateLocalUniforms();
    void CreateTextureUniforms();
    void HandleShaderInjection();
    void RenderNode(wgpu::RenderPassEncoder& renderPass, const Node& node, const glm::mat4& parentTransform);

    std::unique_ptr<tinygltf::Model> m_pModel;
    std::vector<wgpu::Buffer> m_Buffers;
    std::vector<Node> m_Nodes;

    std::unordered_map<std::string, ResourceShaderSharedPtr> m_Shaders;
    int m_DependentResourcesToLoad;
    int m_DependentResourcesLoaded;

    struct PrimitiveRenderData
    {
        struct VertexBufferData
        {
            std::string attribute;
            uint32_t slot;
            uint32_t index;
            uint64_t offset;
            uint64_t count;
        };
        std::vector<VertexBufferData> vertexData;
        
        struct IndexData
        {
            uint32_t bufferIndex;
            uint64_t count;
            wgpu::IndexFormat format;
            uint64_t offset;
        };
        std::optional<IndexData> indexData;

        wgpu::RenderPipeline pipeline;
    };

    using MeshRenderData = std::vector<PrimitiveRenderData>;
    std::vector<MeshRenderData> m_RenderData;

    bool m_IsIndexed;

    struct LocalUniforms
    {
        glm::mat4x4 modelMatrix;
    };
    LocalUniforms m_LocalUniforms;

    wgpu::Buffer m_LocalUniformsBuffer;
    wgpu::BindGroup m_LocalUniformsBindGroup;
    wgpu::BindGroupLayout m_LocalUniformsBindGroupLayout;

    wgpu::BindGroup m_TextureUniformsBindGroup;
    wgpu::BindGroupLayout m_TextureUniformsBindGroupLayout;

    std::optional<SignalId> m_ShaderInjectionSignalId;
    std::vector<ResourceTexture2DUniquePtr> m_Textures;
    std::vector<Material> m_Materials;
};

} // namespace WingsOfSteel::Pandora