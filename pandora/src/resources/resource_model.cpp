#include "resources/resource_model.hpp"

#include "core/log.hpp"
#include "pandora.hpp"
#include "physics/collision_shape.hpp"
#include "render/debug_render.hpp"
#include "render/rendersystem.hpp"
#include "render/window.hpp"
#include "resources/resource_system.hpp"
#include "resources/resource_texture_2d.hpp"

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

// clang-format off
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#include <glm/gtx/quaternion.hpp>
#pragma clang diagnostic pop
#else
#include <glm/gtx/quaternion.hpp>
#endif
// clang-format on

#include <algorithm>
#include <array>
#include <bitset>
#include <functional>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace WingsOfSteel::Pandora
{

static bool sShaderLocationsMapInitialized = false;
static std::unordered_map<std::string, int> sShaderLocationsMap;
static ResourceModel::Id sId = 0;

ResourceModel::ResourceModel()
    : m_DependentResourcesToLoad(0)
    , m_DependentResourcesLoaded(0)
    , m_IsIndexed(false)
    , m_Id(sId++)
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
        [this](FileReadResult result, FileSharedPtr pFile) {
            this->LoadInternal(result, pFile);
        });
}

ResourceType ResourceModel::GetResourceType() const
{
    return ResourceType::Model;
}

void ResourceModel::Render(wgpu::RenderPassEncoder& renderPass, const std::vector<glm::mat4>& instanceTransforms)
{
    m_InstanceCount = std::min(instanceTransforms.size(), MaxInstanceCount);
    std::copy_n(instanceTransforms.begin(), m_InstanceCount, m_InstanceUniforms.data.instanceTransforms.begin());
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_InstanceUniforms.buffer, 0, &m_InstanceUniforms.data, sizeof(InstanceUniformsData));

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_InstanceUniforms.buffer, 0, m_InstanceUniforms.data.instanceTransforms.data(), sizeof(InstanceUniformsData));
    renderPass.SetBindGroup(2, m_InstanceUniforms.bindGroup);

    for (auto& node : m_Nodes)
    {
        if (node.IsRoot())
        {
            // Do not break here: a GLTF scene can have more than one root node.
            RenderNode(renderPass, node, glm::mat4(1.0f));
        }
    }
}

std::optional<ResourceModel::AttachmentPoint> ResourceModel::GetAttachmentPoint(const std::string& name) const
{
    for (auto& attachmentPoint : m_AttachmentPoints)
    {
        if (attachmentPoint.m_Name == name)
        {
            return attachmentPoint;
        }
    }

    return std::nullopt;
}

void ResourceModel::RenderNode(wgpu::RenderPassEncoder& renderPass, const Node& node, const glm::mat4& parentTransform)
{
    if (!node.GetMeshId().has_value() || node.IsCollision())
    {
        return;
    }

    const NodeIndex nodeIndex = node.GetIndex();
    const glm::mat4 transform = parentTransform * node.GetTransform();
    LocalUniforms& localUniforms = m_PerNodeLocalUniforms[nodeIndex];
    localUniforms.data.modelMatrix = transform;
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(localUniforms.buffer, 0, &localUniforms.data, sizeof(LocalUniformsData));
    renderPass.SetBindGroup(1, localUniforms.bindGroup);

    const uint32_t meshId = node.GetMeshId().value();
    for (auto& primitiveRenderData : m_RenderData[meshId])
    {
        if (primitiveRenderData.material.has_value())
        {
            renderPass.SetBindGroup(3, primitiveRenderData.material.value().GetBindGroup());
        }

        renderPass.SetPipeline(primitiveRenderData.pipeline);
        for (const auto& vertexData : primitiveRenderData.vertexData)
        {
            renderPass.SetVertexBuffer(vertexData.slot, m_Buffers[vertexData.index], vertexData.offset);
        }

        if (primitiveRenderData.indexData.has_value())
        {
            renderPass.SetIndexBuffer(m_Buffers[primitiveRenderData.indexData->bufferIndex], primitiveRenderData.indexData->format, primitiveRenderData.indexData->offset);
            renderPass.DrawIndexed(primitiveRenderData.indexData->count, m_InstanceCount);
        }
        else
        {
            renderPass.Draw(primitiveRenderData.vertexData[0].count, m_InstanceCount);
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
        sShaderLocationsMap["COLOR_0"] = 3;
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
            CreateLocalUniformsLayout();
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
            .size = static_cast<size_t>(glm::ceil(static_cast<float>(m_pModel->buffers[i].data.size()) / 4.0f)) * 4, // Must be rounded up to nearest multiple of 4
            .mappedAtCreation = true
        };

        wgpu::Buffer buffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);
        memcpy(buffer.GetMappedRange(), m_pModel->buffers[i].data.data(), m_pModel->buffers[i].data.size());
        buffer.Unmap();
        m_Buffers[i] = buffer;
    }

    SetupMaterials();
    SetupNodes();
    SetupAttachments();
    SetupMeshes();
    SetupCollisionShape();

    SetState(ResourceState::Loaded);

    HandleShaderInjection();
}

void ResourceModel::SetupMaterials()
{
    auto GetTexture = [this](int index) -> ResourceTexture2D* {
        if (index < 0 || index > static_cast<int>(m_Textures.size()))
        {
            return nullptr;
        }
        else
        {
            return m_Textures[static_cast<size_t>(index)].get();
        }
    };

    auto ToVec3 = [](const std::vector<double>& data) {
        assert(data.size() == 3);
        return glm::vec3(data[0], data[1], data[2]);
    };

    auto ToVec4 = [](const std::vector<double>& data) {
        assert(data.size() == 4);
        return glm::vec4(data[0], data[1], data[2], data[3]);
    };

    for (auto& material : m_pModel->materials)
    {
        MaterialSpec spec{
            .baseColorFactor = ToVec4(material.pbrMetallicRoughness.baseColorFactor),
            .metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
            .roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
            .emissiveFactor = ToVec3(material.emissiveFactor),
            .pBaseColorTexture = GetTexture(material.pbrMetallicRoughness.baseColorTexture.index),
            .pMetallicRoughnessTexture = GetTexture(material.pbrMetallicRoughness.metallicRoughnessTexture.index),
            .pNormalTexture = GetTexture(material.normalTexture.index),
            .pOcclusionTexture = GetTexture(material.occlusionTexture.index),
            .pEmissiveTexture = GetTexture(material.emissiveTexture.index)
        };

        m_Materials.emplace_back(spec);
    }
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
    const size_t numNodes = m_pModel->nodes.size();
    for (size_t i = 0; i < numNodes; i++)
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
                glm::quat rotation(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
                nodeTransform = nodeTransform * glm::toMat4(rotation);
            }

            if (node.scale.size() > 0)
            {
                assert(node.scale.size() == 3);
                nodeTransform = nodeTransform * glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
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

        // Any node with the string "Collision" will be converted into a convex collision shape.
        const bool isCollision = node.name.starts_with("Collision");

        m_Nodes.emplace_back(
            static_cast<NodeIndex>(i),
            node.name,
            nodeTransform,
            childNodes,
            rootNodeBitset[i],
            isCollision,
            meshId);
    }

    CreatePerNodeLocalUniforms();
    CreateInstanceUniforms();
}

void ResourceModel::SetupAttachments()
{
    // Find attachment points.
    m_AttachmentPoints.clear();
    std::function<void(const Node&, const glm::mat4&)> findAttachmentPoints;
    findAttachmentPoints =
        [&](const Node& node, const glm::mat4& parentTransform) {
            glm::mat4 modelTransform = parentTransform * node.GetTransform();

            const std::string attachmentPrefix = "Attachment";
            if (node.GetName().starts_with(attachmentPrefix))
            {
                AttachmentPoint ap;
                ap.m_Name = node.GetName().substr(attachmentPrefix.size());
                ap.m_LocalTransform = node.GetTransform();
                ap.m_ModelTransform = modelTransform;
                m_AttachmentPoints.push_back(ap);
            }

            for (const auto& childIndex : node.GetChildren())
            {
                findAttachmentPoints(m_Nodes[childIndex], modelTransform);
            }
        };

    for (const auto& node : m_Nodes)
    {
        if (node.IsRoot())
        {
            findAttachmentPoints(node, glm::mat4(1.0f));
        }
    }
}

// Note that this is called if a relevant shader is injected.
void ResourceModel::SetupMeshes()
{
    const size_t numMeshes = m_pModel->meshes.size();
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

void ResourceModel::SetupCollisionShape()
{
    std::vector<CollisionShapeConvexHullSharedPtr> collisionShapes;
    for (auto& node : m_Nodes)
    {
        if (node.IsCollision())
        {
            auto meshId = node.GetMeshId();
            if (meshId.has_value())
            {
                ConvexHullVertices convexHullVertices;
                auto& mesh = m_pModel->meshes[meshId.value()];
                for (auto& primitive : mesh.primitives)
                {
                    const int positionAttribute = primitive.attributes["POSITION"];
                    const tinygltf::Accessor& accessor = m_pModel->accessors[positionAttribute];
                    const tinygltf::BufferView& bufferView = m_pModel->bufferViews[accessor.bufferView];
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);
                    const uint64_t arrayStride = 3 * sizeof(float);
                    const tinygltf::Buffer& buffer = m_pModel->buffers[bufferView.buffer];
                    const uint8_t* pPositionData = &buffer.data[bufferView.byteOffset];
                    for (size_t i = 0; i < accessor.count; i++)
                    {
                        const float* pData = reinterpret_cast<const float*>(pPositionData);
                        convexHullVertices.push_back(glm::vec3(pData[0], pData[1], pData[2]));
                        pPositionData += arrayStride;
                    }
                }

                collisionShapes.push_back(std::make_shared<CollisionShapeConvexHull>(convexHullVertices));
            }
        }
    }

    const size_t numCollisionShapes = collisionShapes.size();
    if (numCollisionShapes > 1)
    {
        assert(false); // Not implemented yet.
    }
    else if (numCollisionShapes == 1)
    {
        m_pCollisionShape = collisionShapes[0];
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
            if (accessor.type == TINYGLTF_TYPE_VEC2)
            {
                arrayStride = 2 * sizeof(float);
            }
            else if (accessor.type == TINYGLTF_TYPE_VEC3)
            {
                arrayStride = 3 * sizeof(float);
            }
            else
            {
                Log::Error() << "Unsupported accessor type: " << accessor.type;
                continue;
            }
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

        renderData.indexData = PrimitiveRenderData::IndexData{
            .bufferIndex = static_cast<uint32_t>(m_pModel->bufferViews[pPrimitive->indices].buffer),
            .count = accessor.count,
            .format = GetIndexFormat(&accessor),
            .offset = bufferView.byteOffset + accessor.byteOffset
        };
    }

    wgpu::ColorTargetState colorTargetState{
        .format = Pandora::GetWindow()->GetTextureFormat()
    };

    if (pPrimitive->material != -1)
    {
        renderData.material = m_Materials.at(pPrimitive->material);
        colorTargetState.blend = &renderData.material.value().GetBlendState();
    }

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

    std::vector<wgpu::BindGroupLayout> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_LocalUniformsBindGroupLayout,
        m_InstanceUniformsBindGroupLayout
    };

    if (renderData.material.has_value())
    {
        bindGroupLayouts.push_back(renderData.material.value().GetBindGroupLayout());
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .label = GetName().c_str(),
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "ResourceModel",
        .layout = pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .bufferCount = bufferLayouts.size(),
            .buffers = bufferLayouts.data() },
        .primitive = { .topology = GetPrimitiveTopology(pPrimitive), .cullMode = wgpu::CullMode::None },
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
        case 1:
            return wgpu::VertexFormat::Float32;
        case 2:
            return wgpu::VertexFormat::Float32x2;
        case 3:
            return wgpu::VertexFormat::Float32x3;
        case 4:
            return wgpu::VertexFormat::Float32x4;
        };
    }

    Log::Error() << "Unknown vertex format being requested, component type: " << pAccessor->componentType << ", number of components: " << numberOfComponents;
    return wgpu::VertexFormat::Undefined;
}

wgpu::IndexFormat ResourceModel::GetIndexFormat(const tinygltf::Accessor* pAccessor) const
{
    switch (pAccessor->componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return wgpu::IndexFormat::Uint16;
    case TINYGLTF_COMPONENT_TYPE_INT:
        return wgpu::IndexFormat::Uint32;
    default:
        Log::Error() << "Unsupported index format: " << pAccessor->componentType;
        return wgpu::IndexFormat::Undefined;
    };
}

wgpu::PrimitiveTopology ResourceModel::GetPrimitiveTopology(const tinygltf::Primitive* pPrimitive) const
{
    switch (pPrimitive->mode)
    {
    case TINYGLTF_MODE_POINTS:
        return wgpu::PrimitiveTopology::PointList;
    case TINYGLTF_MODE_LINE:
        return wgpu::PrimitiveTopology::LineList;
    case TINYGLTF_MODE_LINE_STRIP:
        return wgpu::PrimitiveTopology::LineStrip;
    case TINYGLTF_MODE_TRIANGLES:
        return wgpu::PrimitiveTopology::TriangleList;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        return wgpu::PrimitiveTopology::TriangleStrip;
    default:
        Log::Error() << "Unsupported primitive topology: " << pPrimitive->mode;
        return wgpu::PrimitiveTopology::Undefined;
    };
}

int ResourceModel::GetNumberOfComponentsForType(int type) const
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR:
        return 1;
    case TINYGLTF_TYPE_VEC2:
        return 2;
    case TINYGLTF_TYPE_VEC3:
        return 3;
    case TINYGLTF_TYPE_VEC4:
        return 4;
    default:
        return 0;
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

void ResourceModel::CreateLocalUniformsLayout()
{
    static_assert(sizeof(LocalUniformsData) % 16 == 0);

    using namespace wgpu;

    BindGroupLayoutEntry bindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer{
            .type = BufferBindingType::Uniform,
            .minBindingSize = sizeof(LocalUniformsData) }
    };

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry
    };
    m_LocalUniformsBindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);
}

void ResourceModel::CreatePerNodeLocalUniforms()
{
    using namespace wgpu;

    m_PerNodeLocalUniforms.reserve(m_Nodes.size());
    for (auto& node : m_Nodes)
    {
        LocalUniforms localUniforms;
        localUniforms.data.modelMatrix = glm::mat4(1.0f);

        BufferDescriptor bufferDescriptor{
            .label = "Local uniforms buffer",
            .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
            .size = sizeof(LocalUniformsData) * m_pModel->nodes.size()
        };

        localUniforms.buffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);

        BindGroupEntry bindGroupEntry{
            .binding = 0,
            .buffer = localUniforms.buffer,
            .offset = 0,
            .size = sizeof(LocalUniformsData)
        };

        BindGroupDescriptor bindGroupDescriptor{
            .layout = m_LocalUniformsBindGroupLayout,
            .entryCount = 1, // Must match bindGroupLayoutDescriptor.entryCount
            .entries = &bindGroupEntry
        };

        localUniforms.bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
        m_PerNodeLocalUniforms.push_back(std::move(localUniforms));
    }
}

void ResourceModel::CreateInstanceUniforms()
{
    using namespace wgpu;

    static_assert(sizeof(InstanceUniformsData) % 16 == 0);

    BindGroupLayoutEntry bindGroupLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
        .buffer{
            .type = BufferBindingType::Uniform,
            .minBindingSize = sizeof(InstanceUniformsData) }
    };

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .label = "Instance uniforms layout",
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry
    };
    m_InstanceUniformsBindGroupLayout = GetRenderSystem()->GetDevice().CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    BufferDescriptor bufferDescriptor{
        .label = "Instance uniforms buffer",
        .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
        .size = sizeof(InstanceUniformsData)
    };

    m_InstanceUniforms.buffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);

    BindGroupEntry bindGroupEntry{
        .binding = 0,
        .buffer = m_InstanceUniforms.buffer,
        .offset = 0,
        .size = sizeof(InstanceUniformsData)
    };

    BindGroupDescriptor bindGroupDescriptor{
        .layout = m_InstanceUniformsBindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };

    m_InstanceUniforms.bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void ResourceModel::HandleShaderInjection()
{
    if (!m_ShaderInjectionSignalId.has_value())
    {
        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                for (auto& shader : m_Shaders)
                {
                    if (shader.second.get() == pResourceShader)
                    {
                        SetupMeshes();
                        return;
                    }
                }
            });
    }
}

} // namespace WingsOfSteel::Pandora
