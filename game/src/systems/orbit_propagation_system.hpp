#pragma once

#include <vector>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>

#include "render/sgp4_compute_pass.hpp"

namespace WingsOfSteel
{

class OrbitalElementsComponent;

class OrbitPropagationSystem : public System
{
public:
    OrbitPropagationSystem();
    ~OrbitPropagationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    static glm::dvec3 CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements);
    static void OrientPlanets(entt::registry& registry, double gmst);
    static double SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations = 10, double tolerance = 1e-10);

private:
    void UpdateGPU(entt::registry& registry);
    void UpdateCPU(entt::registry& registry, double gmst);

    bool m_UseSGP4 = true;

    SGP4ComputePassSharedPtr m_pComputePass;
    std::vector<OrbitalElementsInput> m_OrbitalElements;
};

} // namespace WingsOfSteel
