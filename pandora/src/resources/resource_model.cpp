#include "resources/resource_model.hpp"

#include "core/log.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "vfs/vfs.hpp"
#include "pandora.hpp"

// clang-format off
#define TINYGLTF_IMPLEMENTATION

#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <nlohmann/json.hpp>
#include "tiny_gltf.h"
// clang-format on

#include <sstream>
#include <unordered_map>

namespace WingsOfSteel::Pandora
{

static bool sShaderLocationsMapInitialized = false;
static std::unordered_map<std::string, int> sShaderLocationsMap;

ResourceModel::ResourceModel()
: m_DependentResourcesToLoad(0)
, m_DependentResourcesLoaded(0)
, m_IsIndexed(false)
{

}

ResourceModel::~ResourceModel()
{

}

void ResourceModel::Load(const std::string& path)
{
    Resource::Load(path);

    InitializeShaderLocationsMap();

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile)
        {
            this->LoadInternal(result, pFile);
        }
    );
}

ResourceType ResourceModel::GetResourceType() const
{
    return ResourceType::Model;
}

void ResourceModel::Render(wgpu::RenderPassEncoder& renderPass)
{
    for (auto& primitiveRenderData : m_PrimitiveRenderData)
    {
        renderPass.SetPipeline(primitiveRenderData.pipeline);
        for (const auto& vertexData : primitiveRenderData.vertexData)
        {
            renderPass.SetVertexBuffer(vertexData.slot, m_Buffers[vertexData.index], vertexData.offset);
        }

        if (primitiveRenderData.indexData.has_value())
        {
            renderPass.SetIndexBuffer(m_Buffers[primitiveRenderData.indexData->bufferIndex], primitiveRenderData.indexData->format, primitiveRenderData.indexData->offset);
            renderPass.DrawIndexed(primitiveRenderData.indexData->count);
        }
        else
        {
            renderPass.Draw(primitiveRenderData.vertexData[0].count);
        }
    }
}

void ResourceModel::InitializeShaderLocationsMap()
{
    if (!sShaderLocationsMapInitialized)
    {
        sShaderLocationsMap["POSITION"] = 0;
        sShaderLocationsMap["NORMAL"] = 1;
        sShaderLocationsMapInitialized = true;
    }
}

void ResourceModel::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    using namespace tinygltf;

    if (result == FileReadResult::Ok)
    {
        m_pModel = std::make_unique<Model>();
        TinyGLTF loader;
        std::string err;
        std::string warn;

        bool loadResult = false;
        if (pFile->GetExtension() == "glb")
        {
            loadResult = loader.LoadBinaryFromMemory(m_pModel.get(), &err, &warn, reinterpret_cast<const unsigned char*>(pFile->GetData().data()), pFile->GetData().size());
        }
        else if (pFile->GetExtension() == "gltf")
        {
            loadResult = loader.LoadASCIIFromString(m_pModel.get(), &err, &warn, pFile->GetData().data(), pFile->GetData().size(), "");
        }
        else
        {
            Log::Error() << "Trying to load model with unsupported format: " << pFile->GetExtension();
        }

        if (loadResult)
        {
            LoadDependentResources();
        }
        else
        {
            SetState(ResourceState::Error);
        }
    }
    else
    {
        SetState(ResourceState::Error);   
    }
}

void ResourceModel::LoadDependentResources()
{
    for (auto& material : m_pModel->materials)
    {
        std::stringstream path;
        path << "/shaders/" << material.name << ".wgsl";
        m_Shaders[path.str()] = nullptr;
    }
    m_DependentResourcesToLoad += m_pModel->materials.size();

    if (m_DependentResourcesToLoad == 0)
    {
        OnDependentResourcesLoaded();
    }
    else
    {
        for (auto& it : m_Shaders)
        {
            const std::string& resourcePath = it.first;
            GetResourceSystem()->RequestResource(resourcePath, [this](ResourceSharedPtr pResource) {
                m_Shaders[pResource->GetPath()] = std::dynamic_pointer_cast<Pandora::ResourceShader>(pResource);
                m_DependentResourcesLoaded++;

                if (m_DependentResourcesToLoad == m_DependentResourcesLoaded)
                {
                    OnDependentResourcesLoaded();
                }
            });
        }
    }
}

void ResourceModel::OnDependentResourcesLoaded()
{
    const size_t numBuffers = m_pModel->buffers.size();
    m_Buffers.resize(numBuffers);
    for (size_t i = 0; i < numBuffers; i++)
    {
        wgpu::BufferUsage bufferUsage = wgpu::BufferUsage::CopyDst;
        for (auto& bufferView : m_pModel->bufferViews)
        {
            if (bufferView.buffer == i)
            {
                if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
                {
                    m_IsIndexed = true;
                    bufferUsage |= wgpu::BufferUsage::Index;
                }
                else
                {
                    bufferUsage |= wgpu::BufferUsage::Vertex;
                }
            }
        }

        wgpu::BufferDescriptor bufferDescriptor{
            .usage = bufferUsage,
            .size = static_cast<size_t>(glm::ceil(m_pModel->buffers[i].data.size() / 4)) * 4, // Must be rounded up to nearest multiple of 4
            .mappedAtCreation = true
        };

        wgpu::Buffer buffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);
        memcpy(buffer.GetMappedRange(), m_pModel->buffers[i].data.data(), m_pModel->buffers[i].data.size());
        buffer.Unmap();
        m_Buffers[i] = buffer;
    }

    for (auto& mesh : m_pModel->meshes)
    {
        Log::Info() << "Loading mesh " << mesh.name;
        for (auto& primitive : mesh.primitives)
        {
            SetupPrimitive(&primitive);
        }
    }

    SetState(ResourceState::Loaded); 
}

void ResourceModel::SetupPrimitive(tinygltf::Primitive* pPrimitive)
{
    PrimitiveRenderData renderData;

    std::vector<wgpu::VertexAttribute> vertexAttributes;
    vertexAttributes.resize(pPrimitive->attributes.size());
    size_t vertexAttributeIndex = 0;
    std::vector<wgpu::VertexBufferLayout> bufferLayouts;

    for (auto& attribute : pPrimitive->attributes)
    {
        const std::string& attributeName = attribute.first;
        const int accessorIndex = attribute.second;
        const tinygltf::Accessor& accessor = m_pModel->accessors[accessorIndex];
        const tinygltf::BufferView& bufferView = m_pModel->bufferViews[accessor.bufferView];

        const std::optional<int> shaderLocation = GetShaderLocation(attributeName);
        if (!shaderLocation.has_value())
        {
            Log::Error() << "Unmapped shader attribute location: " << attributeName;
            continue; 
        }

        wgpu::VertexAttribute& vertexAttribute = vertexAttributes[vertexAttributeIndex];
        vertexAttribute.format = GetVertexFormat(&accessor);
        vertexAttribute.offset = 0;
        vertexAttribute.shaderLocation = shaderLocation.value();
        
        uint64_t arrayStride = bufferView.byteStride;
        if (arrayStride == 0)
        {
            arrayStride = 3 * sizeof(float);
        }

        wgpu::VertexBufferLayout bufferLayout{            
            .arrayStride = arrayStride,
            .stepMode = wgpu::VertexStepMode::Vertex,
            .attributeCount = 1,
            .attributes = &vertexAttribute
        };
        bufferLayouts.push_back(std::move(bufferLayout));

        PrimitiveRenderData::VertexBufferData vertexBufferData{
            .attribute = attributeName,
            .slot = static_cast<uint32_t>(vertexAttributeIndex),
            .index = static_cast<uint32_t>(m_pModel->bufferViews[accessor.bufferView].buffer),
            .offset = bufferView.byteOffset + accessor.byteOffset,
            .count = accessor.count
        };
        renderData.vertexData.push_back(std::move(vertexBufferData));
        vertexAttributeIndex++;
    }

    // std::sort(renderData.vertexData.begin(), renderData.vertexData.end(), [](const PrimitiveRenderData::VertexBufferData& a, const PrimitiveRenderData::VertexBufferData& b) {
    //     return a.slot > b.slot;
    // });

    if (pPrimitive->indices != -1)
    {
        assert(pPrimitive->indices >= 0);
        assert(pPrimitive->indices < m_pModel->accessors.size());

        const tinygltf::Accessor& accessor = m_pModel->accessors[pPrimitive->indices];
        const tinygltf::BufferView& bufferView = m_pModel->bufferViews[accessor.bufferView];
        assert(bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);

        renderData.indexData = PrimitiveRenderData::IndexData {
            .bufferIndex = static_cast<uint32_t>(m_pModel->bufferViews[pPrimitive->indices].buffer),
            .count = accessor.count,
            .format = GetIndexFormat(&accessor),
            .offset = bufferView.byteOffset + accessor.byteOffset
        };
    }

    wgpu::ColorTargetState colorTargetState{
        .format = Pandora::GetWindow()->GetTextureFormat()
    };

    wgpu::ShaderModule shaderModule = GetShaderForPrimitive(pPrimitive)->GetShaderModule();
    wgpu::FragmentState fragmentState{
        .module = shaderModule,
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &GetRenderSystem()->GetGlobalUniformsLayout()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .layout = pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .bufferCount = bufferLayouts.size(),
            .buffers = bufferLayouts.data()
        },
        .primitive = {
            .topology = GetPrimitiveTopology(pPrimitive),
            .cullMode = wgpu::CullMode::None
        },
        .fragment = &fragmentState
    };

    if (renderData.indexData.has_value() && descriptor.primitive.topology == wgpu::PrimitiveTopology::TriangleStrip)
    {
        descriptor.primitive.stripIndexFormat = renderData.indexData->format;
    }

    renderData.pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
    m_PrimitiveRenderData.push_back(std::move(renderData));
}

std::optional<int> ResourceModel::GetShaderLocation(const std::string& attributeName) const
{
    auto it = sShaderLocationsMap.find(attributeName);
    if (it == sShaderLocationsMap.end())
    {
        return std::optional<int>();
    }
    else
    {
        return it->second;
    }
}

wgpu::VertexFormat ResourceModel::GetVertexFormat(const tinygltf::Accessor* pAccessor) const
{
    int numberOfComponents = GetNumberOfComponentsForType(pAccessor->type);

    if (pAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        switch (numberOfComponents)
        {
            case 1: return wgpu::VertexFormat::Float32;
            case 2: return wgpu::VertexFormat::Float32x2;
            case 3: return wgpu::VertexFormat::Float32x3;
            case 4: return wgpu::VertexFormat::Float32x4;
        };
    }

    Log::Error() << "Unknown vertex format being requested, component type: " << pAccessor->componentType << ", number of components: " << numberOfComponents;
    return wgpu::VertexFormat::Undefined;
}

wgpu::IndexFormat ResourceModel::GetIndexFormat(const tinygltf::Accessor* pAccessor) const
{
    switch (pAccessor->componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return wgpu::IndexFormat::Uint16;
        case TINYGLTF_COMPONENT_TYPE_INT: return wgpu::IndexFormat::Uint32;
        default: 
            Log::Error() << "Unsupported index format: " << pAccessor->componentType;
            return wgpu::IndexFormat::Undefined;
    };
}

wgpu::PrimitiveTopology ResourceModel::GetPrimitiveTopology(const tinygltf::Primitive* pPrimitive) const
{
    switch (pPrimitive->mode)
    {
        case TINYGLTF_MODE_POINTS: return wgpu::PrimitiveTopology::PointList;
        case TINYGLTF_MODE_LINE: return wgpu::PrimitiveTopology::LineList;
        case TINYGLTF_MODE_LINE_STRIP: return wgpu::PrimitiveTopology::LineStrip;
        case TINYGLTF_MODE_TRIANGLES: return wgpu::PrimitiveTopology::TriangleList;
        case TINYGLTF_MODE_TRIANGLE_STRIP: return wgpu::PrimitiveTopology::TriangleStrip;
        default:
            Log::Error() << "Unsupported primitive topology: " << pPrimitive->mode;
            return wgpu::PrimitiveTopology::Undefined;
    };
}

int ResourceModel::GetNumberOfComponentsForType(int type) const
{
    switch (type)
    {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        default: return 0;
    };
}

ResourceShader* ResourceModel::GetShaderForPrimitive(tinygltf::Primitive* pPrimitive) const
{
    tinygltf::Material& material = m_pModel->materials[pPrimitive->material];
    std::stringstream path;
    path << "/shaders/" << material.name << ".wgsl";
    auto it = m_Shaders.find(path.str());
    return (it == m_Shaders.end()) ? nullptr : it->second.get();
}

} // namespace WingsOfSteel::Pandora
