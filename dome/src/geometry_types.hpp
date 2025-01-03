#pragma once

#include <glm/vec3.hpp>

namespace WingsOfSteel::Dome
{

struct Point {
    int x;
    int y;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct IndexedTriangle
{
    int v0;
    int v1;
    int v2;
};

} // namespace WingsOfSteel::Dome