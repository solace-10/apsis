#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class OrbitalElementsComponent;

class OrbitSimulationSystem : public System
{
public:
    OrbitSimulationSystem();
    ~OrbitSimulationSystem();

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

    static glm::dvec3 CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements);
    static double CalculateGMST();
    static glm::dvec2 ECIToLatLon(const glm::dvec3& eciPosition);
    static double SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations = 10, double tolerance = 1e-10);
};

} // namespace WingsOfSteel
