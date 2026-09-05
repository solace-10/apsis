#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "components/planet_component.hpp"
#include "space/earth_frame.hpp"
#include "space/sgp4.hpp"
#include "systems/orbit_propagation_system.hpp"

namespace WingsOfSteel
{

// Earth's gravitational parameter (km³/s²)
static constexpr double kMu = 398600.4418;

OrbitPropagationSystem::OrbitPropagationSystem()
{
}

OrbitPropagationSystem::~OrbitPropagationSystem()
{
    // The pass list belongs to the RenderSystem and outlives any individual scene, so a
    // pass registered by this system has to be withdrawn by it as well.
    RenderSystem* pRenderSystem = GetRenderSystem();
    if (pRenderSystem && m_pComputePass)
    {
        pRenderSystem->RemovePass(m_pComputePass);
    }
}

void OrbitPropagationSystem::Initialize(Scene* pScene)
{
    m_pComputePass = std::make_shared<SGP4ComputePass>();
    GetRenderSystem()->AddPass(m_pComputePass);
}

void OrbitPropagationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();

    // One instant for the whole frame, and one GMST derived from it, shared by the
    // planet's orientation and by every ground track. Sampling the clock per satellite
    // would put objects resolved early in the frame in a fractionally different frame to
    // those resolved late, and - far more importantly - in a different frame to the planet
    // they are drawn over.
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const double gmst = CalculateGMST(now);

    OrientPlanets(registry, gmst);

    if (m_UseSGP4)
    {
        UpdateGPU(registry);
    }
    else
    {
        UpdateCPU(registry, now, gmst);
    }
}

// Feeds the compute pass the objects to propagate, then applies whatever results have
// come back from an earlier frame's dispatch.
void OrbitPropagationSystem::UpdateGPU(entt::registry& registry)
{
    if (!m_pComputePass->IsReady())
    {
        return;
    }

    UpdateRoster(registry);
    ApplyPropagatedPositions(registry);

    Log::Info() << "Compute pass: " << m_pComputePass->GetResults().positions.size() << " values.";
}

// Builds the set of objects to propagate, and uploads it only when it has changed.
// OrbitalStateComponent is only present on the objects the user can currently see.
void OrbitPropagationSystem::UpdateRoster(entt::registry& registry)
{
    auto view = registry.view<const OrbitalElementsComponent, const OrbitalStateComponent>();

    m_RosterScratch.clear();
    view.each([this](const EntityHandle entityHandle, const OrbitalElementsComponent&, const OrbitalStateComponent&) {
        m_RosterScratch.push_back(entityHandle);
    });

    if (m_pRoster && *m_pRoster == m_RosterScratch)
    {
        return;
    }

    // Everything below here runs only when the roster has actually changed, which is what makes
    // it affordable: initialising SGP4 costs orders of magnitude more than copying six floats,
    // and doing it for the whole visible set every frame would not be. A group filter toggled
    // over thirty thousand objects pays for it once, in single-digit milliseconds.
    m_OrbitalElements.clear();
    m_ElementSets.clear();
    m_OrbitalElements.reserve(m_RosterScratch.size());
    m_ElementSets.reserve(m_RosterScratch.size());

    for (const EntityHandle entityHandle : m_RosterScratch)
    {
        const OrbitalElementsComponent& orbitalElements = view.get<const OrbitalElementsComponent>(entityHandle);

        m_OrbitalElements.push_back(OrbitalElementsInput{
            .meanMotion = orbitalElements.GetMeanMotion(),
            .eccentricity = orbitalElements.GetEccentricity(),
            .inclination = orbitalElements.GetInclination(),
            .raan = orbitalElements.GetRightAscensionOfAscendingNode(),
            .argumentOfPericenter = orbitalElements.GetArgumentOfPericenter(),
            .meanAnomaly = orbitalElements.GetMeanAnomaly() });

        // Parallel to the roster rather than cached against the entity: entt recycles handles, so
        // a map keyed on one would eventually answer for an object that no longer exists.
        m_ElementSets.push_back(SGP4Initialise(MakeSGP4Elements(orbitalElements)));
    }

    // A new roster rather than a mutated one, so that a readback still in flight keeps
    // the roster it was dispatched with and stays able to say whose positions it holds.
    m_pRoster = std::make_shared<const EntityRoster>(m_RosterScratch);
    m_pComputePass->SetOrbitalElements(m_OrbitalElements, m_pRoster);
}

// Writes back the positions of the last completed readback.
//
// The results describe the roster they were dispatched with, which need not be the one
// currently uploaded - a group filter toggled in the intervening frames does not
// invalidate them, as where an object is has nothing to do with which groups are
// enabled. Objects that have since left the tracked set are skipped rather than the
// whole batch being discarded.
void OrbitPropagationSystem::ApplyPropagatedPositions(entt::registry& registry)
{
    const PropagationResults& results = m_pComputePass->GetResults();
    if (!results.pEntities || results.time == m_LastAppliedResultsTime)
    {
        return;
    }
    m_LastAppliedResultsTime = results.time;

    // The instant the positions are valid for, not the current one: the dispatch that
    // produced them is a couple of frames old, and the ground underneath has turned
    // since.
    const double gmst = CalculateGMST(results.time);

    const EntityRoster& entities = *results.pEntities;
    for (size_t i = 0; i < entities.size(); i++)
    {
        const EntityHandle entityHandle = entities[i];

        // entt recycles entity indices, so an object destroyed while the readback was in
        // flight can have handed its index to a different object entirely. The handle
        // carries a version, which is what makes this a real identity check rather than
        // a bounds test.
        if (!registry.valid(entityHandle))
        {
            continue;
        }

        OrbitalStateComponent* pOrbitalState = registry.try_get<OrbitalStateComponent>(entityHandle);
        TransformComponent* pTransform = registry.try_get<TransformComponent>(entityHandle);
        const OrbitalElementsComponent* pOrbitalElements = registry.try_get<OrbitalElementsComponent>(entityHandle);
        if (!pOrbitalState || !pTransform || !pOrbitalElements)
        {
            continue;
        }

        const glm::dvec3 position(results.positions[i]); // Position is in km, in ECI coordinates
        pTransform->transform = glm::translate(glm::mat4(1.0f), glm::vec3(ECIToWorld(position)));
        UpdateOrbitalState(*pOrbitalState, *pOrbitalElements, position, gmst);
    }
}

void OrbitPropagationSystem::UpdateCPU(entt::registry& registry, const std::chrono::system_clock::time_point& instant, double gmst)
{
    auto view = registry.view<const OrbitalElementsComponent, OrbitalStateComponent, TransformComponent>();

    view.each([&instant, gmst](const OrbitalElementsComponent& orbitalElements, OrbitalStateComponent& orbitalState, TransformComponent& transformComponent) {
        const glm::dvec3 position = CalculateCartesianPosition(orbitalElements, instant); // Position is in km, in ECI coordinates
        transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(ECIToWorld(position)));
        UpdateOrbitalState(orbitalState, orbitalElements, position, gmst);
    });
}

// The state that follows from a position, whichever path produced it. Shared so the two
// cannot drift: the readouts they feed are the same readouts.
void OrbitPropagationSystem::UpdateOrbitalState(OrbitalStateComponent& orbitalState, const OrbitalElementsComponent& orbitalElements, const glm::dvec3& positionECI, double gmst)
{
    orbitalState.m_PositionECI = positionECI;

    // Calculate semi-major axis from mean motion: n = sqrt(mu/a³) => a = (mu/n²)^(1/3)
    const double n = orbitalElements.GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;
    orbitalState.m_SemiMajorAxis = std::cbrt(kMu / (n * n));

    const double r = glm::length(positionECI);

    // Calculate velocity from vis-viva equation: v² = μ(2/r - 1/a)
    orbitalState.m_Velocity = std::sqrt(kMu * (2.0 / r - 1.0 / orbitalState.m_SemiMajorAxis));

    const glm::dvec3 geodetic = ECIToGeodetic(positionECI, gmst);
    orbitalState.m_Latitude = geodetic.x;
    orbitalState.m_Longitude = geodetic.y;
    orbitalState.m_Altitude = geodetic.z;
}

// Turns every planet mesh so that its prime meridian sits at the current Greenwich
// Mean Sidereal Time, which is what puts a satellite over the ground it is
// actually above. CalculatePlanetRotation() carries the reasoning behind the
// angle; this is only the ECS plumbing for it.
//
// Only the surface needs this. A rotation about the polar axis maps the oblate
// spheroid exactly onto itself, so the atmosphere shell and the wireframe overlay
// carry no longitude to be wrong about and are deliberately left in world space.
void OrbitPropagationSystem::OrientPlanets(entt::registry& registry, double gmst)
{
    const glm::mat4 rotation(CalculatePlanetRotation(gmst));

    auto view = registry.view<const PlanetComponent, TransformComponent>();
    view.each([&rotation](const PlanetComponent&, TransformComponent& transformComponent) {
        transformComponent.transform = rotation;
    });
}

// Calculate Cartesian position (in km) from Keplerian orbital elements, propagated to the
// given instant.
//
// The instant is a parameter rather than a call to the clock inside here, so that every
// satellite in a frame is propagated to the same one - and to the same one the planet is
// oriented with - and so that the result is something a test can predict.
glm::dvec3 OrbitPropagationSystem::CalculateCartesianPosition(const OrbitalElementsComponent& orbitalElements, const std::chrono::system_clock::time_point& instant)
{
    // Convert mean motion from rev/day to rad/s
    const double n = orbitalElements.GetMeanMotion() * 2.0 * glm::pi<double>() / 86400.0;

    // Calculate semi-major axis from mean motion: n = sqrt(mu/a³) => a = (mu/n²)^(1/3)
    const double a = std::cbrt(kMu / (n * n));

    const double e = orbitalElements.GetEccentricity();

    // Convert angles from degrees to radians
    const double i = glm::radians(static_cast<double>(orbitalElements.GetInclination()));
    const double omega = glm::radians(static_cast<double>(orbitalElements.GetRightAscensionOfAscendingNode())); // RAAN (Ω)
    const double w = glm::radians(static_cast<double>(orbitalElements.GetArgumentOfPericenter())); // Argument of pericenter (ω)
    const double M_epoch = glm::radians(static_cast<double>(orbitalElements.GetMeanAnomaly()));

    // Propagate mean anomaly to the given time
    const auto epoch = orbitalElements.GetEpoch();
    const double deltaSeconds = std::chrono::duration<double>(instant - epoch).count();
    double M = M_epoch + n * deltaSeconds;

    // Normalize to [0, 2π]
    M = std::fmod(M, 2.0 * glm::pi<double>());
    if (M < 0.0)
    {
        M += 2.0 * glm::pi<double>();
    }

    // Solve Kepler's equation to get Eccentric Anomaly
    const double E = SolveKeplerEquation(M, e);

    // Calculate True Anomaly from Eccentric Anomaly
    // tan(nu/2) = sqrt((1+e)/(1-e)) * tan(E/2)
    const double nu = 2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(E / 2.0), std::sqrt(1.0 - e) * std::cos(E / 2.0));

    // Calculate orbital radius
    const double r = a * (1.0 - e * std::cos(E));

    // Position in perifocal (orbital plane) coordinates
    const double x_pf = r * std::cos(nu);
    const double y_pf = r * std::sin(nu);

    // Transform from perifocal to ECI (Earth-Centered Inertial) coordinates
    // Using the rotation: R = R_z(Ω) * R_x(i) * R_z(ω)
    const double cos_omega = std::cos(omega);
    const double sin_omega = std::sin(omega);
    const double cos_i = std::cos(i);
    const double sin_i = std::sin(i);
    const double cos_w = std::cos(w);
    const double sin_w = std::sin(w);

    // Combined rotation matrix elements (optimized form)
    const double x = x_pf * (cos_omega * cos_w - sin_omega * sin_w * cos_i)
        + y_pf * (-cos_omega * sin_w - sin_omega * cos_w * cos_i);

    const double y = x_pf * (sin_omega * cos_w + cos_omega * sin_w * cos_i)
        + y_pf * (-sin_omega * sin_w + cos_omega * cos_w * cos_i);

    const double z = x_pf * (sin_w * sin_i)
        + y_pf * (cos_w * sin_i);

    return glm::dvec3(x, y, z);
}

// Solve Kepler's equation: M = E - e*sin(E)
// Returns Eccentric Anomaly E given Mean Anomaly M and eccentricity e
// Uses Newton-Raphson iteration
double OrbitPropagationSystem::SolveKeplerEquation(double meanAnomaly, double eccentricity, int maxIterations, double tolerance)
{
    double E = meanAnomaly; // Initial guess
    for (int i = 0; i < maxIterations; ++i)
    {
        const double f = E - eccentricity * std::sin(E) - meanAnomaly;
        const double fPrime = 1.0 - eccentricity * std::cos(E);
        const double delta = f / fPrime;
        E -= delta;
        if (std::abs(delta) < tolerance)
        {
            break;
        }
    }
    return E;
}

} // namespace WingsOfSteel
