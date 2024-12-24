#pragma once

namespace WingsOfSteel::Dome
{

struct Color
{
    float r;
    float g;
    float b;
};

struct Point {
    int x;
    int y;
};

struct Vertex {
    int x;
    int y;
    Color color;
};

struct IndexedTriangle
{
    int v0;
    int v1;
    int v2;
};

} // namespace WingsOfSteel::Dome