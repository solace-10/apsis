#pragma once

#include <cstdint>

namespace WingsOfSteel
{

class PlanetComponent;

class PlanetMeshGenerator
{
public:
    static void Generate(PlanetComponent& component, uint32_t subdivisions = 16);
};

} // namespace WingsOfSteel
