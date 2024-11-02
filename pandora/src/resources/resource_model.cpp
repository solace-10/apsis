#include "resources/resource_model.hpp"

#include "core/log.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "resources/resource_texture_2d.hpp"
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

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <array>
#include <bitset>
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
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
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

void ResourceModel::Render(wgpu::RenderPassEncoder& renderPass, const glm::mat4& transform)
{
    for (auto& node : m_Nodes)
    {
        if (node.IsRoot())
        {
            RenderNode(renderPass, node, transform);
        }
    }

}

void ResourceModel::RenderNode(wgpu::RenderPassEncoder& renderPass, const Node& node, const glm::mat4& parentTransform)
{
    if (!node.GetMeshId().has_value())
    {
        return;
    }

    const uint32_t nodeId = node.GetMeshId().value();
    const glm::mat4 transform = parentTransform * node.GetTransform();

    for (auto& primitiveRenderData : m_RenderData[nodeId])
    {
        m_LocalUniforms.modelMatrix = transform;
        GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_LocalUniformsBuffer, 0, &m_LocalUniforms, sizeof(LocalUniforms));
        renderPass.SetBindGroup(1, m_LocalUniformsBindGroup);

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

    for (auto& childNodeId : node.GetChildren())
    {
        const Node& childNode = m_Nodes[childNodeId];
        RenderNode(renderPass, childNode, transform);
    }
}

void ResourceModel::InitializeShaderLocationsMap()
{
    if (!sShaderLocationsMapInitialized)
    {
        sShaderLocationsMap["POSITION"] = 0;
        sShaderLocationsMap["NORMAL"] = 1;
        sShaderLocationsMap["TEXCOORD_0"] = 2;
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
            CreateLocalUniforms();
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
    m_DependentResourcesToLoad += m_pModel->images.size();

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

        const size_t numImages = m_pModel->images.size();
        m_Textures.resize(numImages);
        for (size_t i = 0; i < numImages; i++)
        {
            auto& image = m_pModel->images[i];

            // According to the spec (https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-image), bufferView will be set if the texture is
            // built into the GLTF rather than an external file.
            if (image.bufferView >= 0)
            {
                auto& bufferView = m_pModel->bufferViews[image.bufferView];
                auto& buffer = m_pModel->buffers[bufferView.buffer];
                const std::string label = GetPath() + "[" + image.name + "]"; 
                m_Textures[i] = std::make_unique<ResourceTexture2D>(label, &buffer.data[bufferView.byteOffset], bufferView.byteLength);
                m_DependentResourcesLoaded++;
            }
            else if (!image.uri.empty()) // If we have a URI, then the texture is a local file. Path must be relative to the model file.
            {
                Log::Error() << "Loading textures from URIs is not implemented yet.";
            }
            else
            {
                Log::Error() << "Invalid GLTF file - image definition contains neither a buffer view or a URI.";
            }
        }

        if (m_DependentResourcesToLoad == m_DependentResourcesLoaded)
        {
            OnDependentResourcesLoaded();
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

    SetupNodes();
    SetupMeshes();

    SetState(ResourceState::Loaded);

    HandleShaderInjection();
}

void ResourceModel::SetupNodes()
{
    // Figure out which nodes are root nodes. A root node is a node that has no parent.
    // If a node is a child of another node, then it can't be a root node.
    static const uint32_t sMaxNodes = 32;
    std::bitset<sMaxNodes> rootNodeBitset;
    rootNodeBitset.set(); // Set all bits to 1.
    assert(m_pModel->nodes.size() <= sMaxNodes);
    for (auto& node : m_pModel->nodes)
    {
        for (auto& childNodeId : node.children)
        {
            rootNodeBitset[childNodeId] = false;
        }
    }

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#transformations
    for (size_t i = 0; i < m_pModel->nodes.size(); i++)
    {
        const tinygltf::Node& node = m_pModel->nodes[i]; 
        glm::mat4 nodeTransform(1.0f);

        if (node.matrix.size() > 0)
        {
            assert(node.matrix.size() == 16);

            // glTF has the values has doubles, but we need them as floats.
            std::array<float, 16> values;
            for (int i = 0; i < values.size(); i++)
            {
                values[i] = static_cast<float>(node.matrix[i]);
            }

            nodeTransform = glm::make_mat4(values.data());
        }
        else
        {
            if (node.translation.size() > 0)
            {
                assert(node.translation.size() == 3);
                nodeTransform = glm::translate(nodeTransform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
            }

            if (node.rotation.size() > 0)
            {
                assert(node.rotation.size() == 4); // Rotation is a quaternion.
                // glTF node data has quaternions in XYZW format, but glm::quat expects them in WXYZ.
                nodeTransform = glm::toMat4(glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
            }

            if (node.scale.size() > 0)
            {
                assert(node.scale.size() == 3);
                nodeTransform = glm::scale(nodeTransform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
            }
        }

        NodeIndices childNodes;
        for (const int& childNodeId : node.children)
        {
            childNodes.push_back(static_cast<uint32_t>(childNodeId));
        }

        std::optional<uint32_t> meshId{};
        if (node.mesh != -1)
        {
            meshId = static_cast<uint32_t>(node.mesh);
        }

        m_Nodes.emplace_back(
            node.name,
            nodeTransform,
            childNodes,
            rootNodeBitset[i],
            meshId
        );
    }
}

// Note that this is called if a relevant shader is injected.
void ResourceModel::SetupMeshes()
{
    size_t numMeshes = m_pModel->meshes.size();
    m_RenderData.clear();
    m_RenderData.resize(numMeshes);
    
    for (uint32_t meshId = 0; meshId < numMeshes; meshId++)
    {
        auto& mesh = m_pModel->meshes[meshId];
        for (auto& primitive : mesh.primitives)
        {
            SetupPrimitive(meshId, &primitive);
        }
    }
}

void ResourceModel::SetupPrimitive(uint32_t meshId, tinygltf::Primitive* pPrimitive)
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

    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less
    };

    wgpu::BindGroupLayout bindGroupLayouts[] = 
    {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_LocalUniformsBindGroupLayout
    };

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 2,
        .bindGroupLayouts = bindGroupLayouts
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "ResourceModel",
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
        .depthStencil = &depthState,
        .fragment = &fragmentState
    };

    if (renderData.indexData.has_value() && descriptor.primitive.topology == wgpu::PrimitiveTopology::TriangleStrip)
    {
        descriptor.primitive.stripIndexFormat = renderData.indexData->format;
    }

    renderData.pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);

    m_RenderData[meshId].push_back(std::move(renderData));
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

void ResourceModel::CreateLocalUniforms()
{
    static_assert(sizeof(LocalUniforms) % 16 == 0);

    using namespace wgpu;

    memset(&m_LocalUniforms, 0, sizeof(LocalUniforms));
    m_LocalUniforms.modelMatrix = glm::mat4x4(1.0f);

    BufferDescriptor bufferDescriptor{
        .label = "Local uniforms buffer",
        .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
        .size = sizeof(LocalUniforms)
    };

    m_LocalUniformsBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);

    BindGroupLayoutEntry bindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(LocalUniforms)
        }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry
    };
    m_LocalUniformsBindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    BindGroupEntry bindGroupEntry{
        .binding = 0,
        .buffer = m_LocalUniformsBuffer,
        .offset = 0,
        .size = sizeof(LocalUniforms)
    };

    BindGroupDescriptor bindGroupDescriptor{
        .layout = m_LocalUniformsBindGroupLayout,
        .entryCount = bindGroupLayoutDescriptor.entryCount,
        .entries = &bindGroupEntry
    };

    m_LocalUniformsBindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void ResourceModel::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader)
            {
                for (auto& shader : m_Shaders)
                {
                    if (shader.second.get() == pResourceShader)
                    {
                        SetupMeshes();
                        return;
                    }
                }
            }
        );
    }
}

} // namespace WingsOfSteel::Pandora
