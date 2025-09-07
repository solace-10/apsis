#pragma once

#include <glm/vec3.hpp>

namespace WingsOfSteel
{

struct Point {
    int x{0};
    int y{0};
};

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
};

struct IndexedTriangle
{
    int v0{0};
    int v1{0};
    int v2{0};
};

} // namespace WingsOfSteel