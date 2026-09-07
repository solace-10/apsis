#pragma once

#include <chrono>
#include <vector>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>

#include "render/sgp4_compute_pass.hpp"

namespace WingsOfSteel
{

class OrbitalStateComponent;

class OrbitPropagationSystem : public System
{
public:
    OrbitPropagationSystem();
    ~OrbitPropagationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    static void OrientPlanets(entt::registry& registry, double gmst);

private:
    void UpdateRoster(entt::registry& registry);
    void UpdateTimes(entt::registry& registry, const std::chrono::system_clock::time_point& instant);
    void ApplyPropagatedPositions(entt::registry& registry);
    static void UpdateOrbitalState(OrbitalStateComponent& orbitalState, const glm::dvec3& positionECI, double gmst, double speed);

    SGP4ComputePassSharedPtr m_pComputePass;

    // The roster currently uploaded, and the scratch buffers it is rebuilt into each
    // frame so that the comparison against it does not allocate.
    EntityRosterSharedPtr m_pRoster;
    EntityRoster m_RosterScratch;
    std::vector<SGP4StepInput> m_OrbitalElements;

    // Minutes from each object's epoch, in roster order, rebuilt every frame. One float per
    // object against the coefficients' hundred and forty-four bytes, which is why they are
    // uploaded separately.
    std::vector<float> m_Times;

    // Results are applied once, when they land, rather than rewritten every frame.
    std::chrono::system_clock::time_point m_LastAppliedResultsTime;
};

} // namespace WingsOfSteel
