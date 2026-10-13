#include "render/vertex_buffer_schemas.hpp"
#include "render/vertex_types.hpp"

namespace WingsOfSteel
{

VertexBufferSchemas::VertexBufferSchemas()
{
    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3_C3);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP3C3),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P2_C3_UV);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 2 * sizeof(float),
                .shaderLocation = 1 });
        schema.attributes.push_back( // UV
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 5 * sizeof(float),
                .shaderLocation = 2 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP2C3UV),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P2_C4_UV);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x4,
                .offset = 2 * sizeof(float),
                .shaderLocation = 1 });
        schema.attributes.push_back( // UV
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 6 * sizeof(float),
                .shaderLocation = 2 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP2C4UV),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }
    
    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3_C4_UV);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x4,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1 });
        schema.attributes.push_back( // UV
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 7 * sizeof(float),
                .shaderLocation = 2 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP3C4UV),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3_C3_N3);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1 });
        schema.attributes.push_back( // Normal
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 6 * sizeof(float),
                .shaderLocation = 2 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP3C3N3),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3_B3);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Barycentric
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP3B3),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3_N3_UV);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });
        schema.attributes.push_back( // Normal
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1 });
        schema.attributes.push_back( // UV
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 6 * sizeof(float),
                .shaderLocation = 2 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(VertexP3N3UV),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }

    {
        Schema& schema = GetSchema(VertexFormat::VERTEX_FORMAT_P3);
        schema.attributes.push_back( // Position
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0 });

        schema.layout = {
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(glm::vec3),
            .attributeCount = schema.attributes.size(),
            .attributes = schema.attributes.data()
        };
    }
}

const wgpu::VertexBufferLayout* VertexBufferSchemas::GetLayout(VertexFormat vertexFormat) const
{
    const size_t index = static_cast<size_t>(vertexFormat);
    return &(m_Schemas[index].layout);
}

VertexBufferSchemas::Schema& VertexBufferSchemas::GetSchema(VertexFormat vertexFormat)
{
    return m_Schemas[static_cast<size_t>(vertexFormat)];
}

} // namespace WingsOfSteel
