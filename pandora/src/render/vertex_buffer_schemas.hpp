#pragma once

#include <array>
#include <vector>

#include <webgpu/webgpu_cpp.h>

#include "render/vertex_types.hpp"

namespace WingsOfSteel
{

class VertexBufferSchemas
{
public:
    VertexBufferSchemas();
    ~VertexBufferSchemas() = default;

    const wgpu::VertexBufferLayout* GetLayout(VertexFormat vertexFormat) const;

private:
    struct Schema
    {
        std::vector<wgpu::VertexAttribute> attributes;
        wgpu::VertexBufferLayout layout;
    };

    Schema& GetSchema(VertexFormat vertexFormat);

    std::array<Schema, static_cast<size_t>(VertexFormat::VERTEX_FORMAT_COUNT)> m_Schemas;
};

} // namespace WingsOfSteel
