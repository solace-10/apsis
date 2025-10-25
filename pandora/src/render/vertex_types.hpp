#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace WingsOfSteel
{

enum class VertexFormat
{
    VERTEX_FORMAT_P3_C3 = 0,
    VERTEX_FORMAT_P2_C3_UV,
    VERTEX_FORMAT_P3_C4_UV,
    
    VERTEX_FORMAT_COUNT
};

struct VertexP3C3
{
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexP2C3UV
{
    glm::vec2 position;
    glm::vec3 color;
    glm::vec2 uv;
};

struct VertexP3C4UV
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 uv;    
};

} // namespace WingsOfSteel
