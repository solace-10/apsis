#pragma once

#include "resources/resource.hpp"

#include <atomic>
#include <optional>
#include <unordered_map>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include "resources/resource_shader.hpp"
#include "vfs/vfs.hpp"

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

    void Render(wgpu::RenderPassEncoder renderPass);

private:
    void InitializeShaderLocationsMap();
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);
    void LoadDependentResources();
    void OnDependentResourcesLoaded();
    void SetupPrimitive(tinygltf::Primitive* pPrimitive);
    std::optional<int> GetShaderLocation(const std::string& attributeName) const;
    wgpu::IndexFormat GetIndexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::VertexFormat GetVertexFormat(const tinygltf::Accessor* pAccessor) const;
    wgpu::PrimitiveTopology GetPrimitiveTopology(const tinygltf::Primitive* pPrimitive) const;
    int GetNumberOfComponentsForType(int type) const;
    ResourceShader* GetShaderForPrimitive(tinygltf::Primitive* pPrimitive) const;

    std::unique_ptr<tinygltf::Model> m_pModel;
    std::vector<wgpu::Buffer> m_Buffers;

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
    std::vector<PrimitiveRenderData> m_PrimitiveRenderData;

    bool m_IsIndexed;
};

} // namespace WingsOfSteel::Pandora