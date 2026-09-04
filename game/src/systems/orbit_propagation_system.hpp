#pragma once

#include <chrono>
#include <vector>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>

#include "render/sgp4_compute_pass.hpp"

namespace WingsOfSteel
{

class OrbitalElementsComponent;
class OrbitalStateComponent;

class OrbitPropagationSystem : public System
{
public:
    OrbitPropagationSystem();
    ~OrbitPropagationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    static glm::dvec3 CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements, const std::chrono::system_clock::time_point& instant);
    static void OrientPlanets(entt::registry& registry, double gmst);
    static double SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations = 10, double tolerance = 1e-10);

private:
    void UpdateGPU(entt::registry& registry);
    void UpdateRoster(entt::registry& registry);
    void ApplyPropagatedPositions(entt::registry& registry);
    void UpdateCPU(entt::registry& registry, const std::chrono::system_clock::time_point& instant, double gmst);
    static void UpdateOrbitalState(OrbitalStateComponent& orbitalState, const OrbitalElementsComponent& orbitalElements, const glm::dvec3& positionECI, double gmst);

    bool m_UseSGP4 = true;

    SGP4ComputePassSharedPtr m_pComputePass;

    // The roster currently uploaded, and the scratch buffers it is rebuilt into each
    // frame so that the comparison against it does not allocate.
    EntityRosterSharedPtr m_pRoster;
    EntityRoster m_RosterScratch;
    std::vector<OrbitalElementsInput> m_OrbitalElements;

    // Results are applied once, when they land, rather than rewritten every frame.
    std::chrono::system_clock::time_point m_LastAppliedResultsTime;
};

} // namespace WingsOfSteel
