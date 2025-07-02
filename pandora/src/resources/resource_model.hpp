#pragma once

#include "resources/resource.fwd.hpp"
#include "resources/resource.hpp"

#include <array>
#include <optional>
#include <string>
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
} // namespace tinygltf

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(CollisionShape);

class ResourceModel : public Resource
{
public:
    class AttachmentPoint
    {
    public:
        std::string m_Name;
        glm::mat4 m_LocalTransform;
        glm::mat4 m_ModelTransform;
    };

    using Id = uint32_t;
    using InstanceTransforms = std::vector<glm::mat4>;

    ResourceModel();
    ~ResourceModel() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    void Render(wgpu::RenderPassEncoder& renderPass, const InstanceTransforms& instanceTransforms);

    const std::vector<AttachmentPoint>& GetAttachmentPoints() const { return m_AttachmentPoints; }
    std::optional<AttachmentPoint> GetAttachmentPoint(const std::string& name) const;
    CollisionShapeSharedPtr GetCollisionShape() const { return m_pCollisionShape; }

    Id GetId() const { return m_Id; }
    static constexpr size_t MaxInstanceCount = 256;

private:
    using NodeIndex = uint32_t;
    using NodeIndices = std::vector<NodeIndex>;
    class Node
    {
    public:
        Node(NodeIndex index, const std::string& name, const glm::mat4& transform, const NodeIndices& children, bool isRoot, bool isCollision, std::optional<uint32_t> meshId)
            : m_Index(index)
            , m_Transform(transform)
            , m_Name(name)
            , m_Children(children)
            , m_IsRoot(isRoot)
            , m_IsCollision(isCollision)
            , m_MeshId(meshId)
        {
        }

        ~Node() {}

        inline NodeIndex GetIndex() const { return m_Index; }
        inline const glm::mat4 GetTransform() const { return m_Transform; }
        inline const std::string& GetName() const { return m_Name; }
        inline const NodeIndices& GetChildren() const { return m_Children; }
        inline bool IsRoot() const { return m_IsRoot; }
        inline bool IsCollision() const { return m_IsCollision; }
        inline std::optional<uint32_t> GetMeshId() const { return m_MeshId; }

    private:
        NodeIndex m_Index{ 0 };
        glm::mat4 m_Transform{ 1.0f };
        std::string m_Name;
        NodeIndices m_Children;
        bool m_IsRoot{ false };
        bool m_IsCollision{ false };
        std::optional<uint32_t> m_MeshId;
    };

    void InitializeShaderLocationsMap();
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void LoadDependentResources();
    void OnDependentResourcesLoaded();
    void SetupMaterials();
    void SetupNodes();
    void SetupAttachments();
    void SetupMeshes();
    void SetupPrimitive(uint32_t meshId, tinygltf::Primitive* pPrimitive);
    void SetupCollisionShape();
    std::optional<int> GetShaderLocation(const std::string& attributeName) const;
    wgpu::IndexFormat GetIndexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::VertexFormat GetVertexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::PrimitiveTopology GetPrimitiveTopology(const tinygltf::Primitive* pPrimitive) const;
    int GetNumberOfComponentsForType(int type) const;
    ResourceShader* GetShaderForPrimitive(tinygltf::Primitive* pPrimitive) const;
    void CreateLocalUniformsLayout();
    void CreatePerNodeLocalUniforms();
    void CreateInstanceUniforms();
    void CreateTextureUniforms();
    void HandleShaderInjection();
    void RenderNode(wgpu::RenderPassEncoder& renderPass, const Node& node, const glm::mat4& parentTransform);

    std::unique_ptr<tinygltf::Model> m_pModel;
    std::vector<wgpu::Buffer> m_Buffers;
    std::vector<Node> m_Nodes;
    std::vector<AttachmentPoint> m_AttachmentPoints;

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

        std::optional<Material> material;
        wgpu::RenderPipeline pipeline;
    };

    using MeshRenderData = std::vector<PrimitiveRenderData>;
    std::vector<MeshRenderData> m_RenderData;

    bool m_IsIndexed;

    struct LocalUniformsData
    {
        glm::mat4x4 modelMatrix;
    };

    struct LocalUniforms
    {
        LocalUniformsData data;
        wgpu::Buffer buffer;
        wgpu::BindGroup bindGroup;
    };
    std::vector<LocalUniforms> m_PerNodeLocalUniforms;
    wgpu::BindGroupLayout m_LocalUniformsBindGroupLayout;

    struct InstanceUniformsData
    {
        std::array<glm::mat4, ResourceModel::MaxInstanceCount> instanceTransforms;
    };

    struct InstanceUniforms
    {
        uint32_t numInstances{ 0 };
        InstanceUniformsData data;
        wgpu::Buffer buffer;
        wgpu::BindGroup bindGroup;
    };
    InstanceUniforms m_InstanceUniforms;
    wgpu::BindGroupLayout m_InstanceUniformsBindGroupLayout;
    uint32_t m_InstanceCount{ 0 };

    wgpu::BindGroup m_TextureUniformsBindGroup;
    wgpu::BindGroupLayout m_TextureUniformsBindGroupLayout;

    std::optional<SignalId> m_ShaderInjectionSignalId;
    std::vector<ResourceTexture2DUniquePtr> m_Textures;
    std::vector<Material> m_Materials;

    CollisionShapeSharedPtr m_pCollisionShape;
    Id m_Id{ 0 };
};

} // namespace WingsOfSteel::Pandora