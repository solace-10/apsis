#include "render/vertex_buffer_schemas.hpp"

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
                .shaderLocation = 0
            }
        );
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1
            }
        );

        schema.layout = {
            .arrayStride = sizeof(VertexP3C3),
            .stepMode = wgpu::VertexStepMode::Vertex,
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
                .shaderLocation = 0
            }
        );
        schema.attributes.push_back( // Color
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 2 * sizeof(float),
                .shaderLocation = 1
            }
        );
        schema.attributes.push_back( // UV
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 5 * sizeof(float),
                .shaderLocation = 2
            }
        );

        schema.layout = {
            .arrayStride = sizeof(VertexP2C3UV),
            .stepMode = wgpu::VertexStepMode::Vertex,
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
