#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/orbital_elements_component.hpp"
#include "components/orbital_state_component.hpp"
#include "components/planet_component.hpp"
#include "components/propagation_failure_component.hpp"
#include "components/sgp4_component.hpp"
#include "space/earth_frame.hpp"
#include "systems/orbit_propagation_system.hpp"

namespace WingsOfSteel
{

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

    // One instant for the whole frame. Sampling the clock per satellite would put objects
    // resolved early in the frame in a fractionally different frame to those resolved late.
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const double gmst = CalculateGMST(now);

    OrientPlanets(registry, gmst);

    if (!m_pComputePass->IsReady())
    {
        return;
    }

    UpdateRoster(registry);
    UpdateTimes(registry, now);
    ApplyPropagatedPositions(registry);
}

// Builds the set of objects to propagate, and uploads it only when it has changed.
// Every object the user can currently see, which is what OrbitalStateComponent marks.
void OrbitPropagationSystem::UpdateRoster(entt::registry& registry)
{
    auto view = registry.view<const SGP4Component, const OrbitalStateComponent>();

    m_RosterScratch.clear();
    view.each([this](const EntityHandle entityHandle, const SGP4Component&, const OrbitalStateComponent&) {
        m_RosterScratch.push_back(entityHandle);
    });

    if (m_pRoster && *m_pRoster == m_RosterScratch)
    {
        return;
    }

    // Everything below here runs only when the roster has actually changed. The coefficients are
    // fixed for the life of the object, so re-packing thirty thousand of them every frame to hand
    // back a buffer identical to the one already on the GPU would be work for nothing.
    m_OrbitalElements.clear();
    m_OrbitalElements.reserve(m_RosterScratch.size());

    for (const EntityHandle entityHandle : m_RosterScratch)
    {
        const SGP4Component& sgp4Component = view.get<const SGP4Component>(entityHandle);
        m_OrbitalElements.push_back(MakeSGP4StepInput(sgp4Component.m_ElementSet));
    }

    // A new roster rather than a mutated one, so that a readback still in flight keeps
    // the roster it was dispatched with and stays able to say whose positions it holds.
    m_pRoster = std::make_shared<const EntityRoster>(m_RosterScratch);
    m_pComputePass->SetOrbitalElements(m_OrbitalElements, m_pRoster);
}

// Writes back the positions of the last completed readback.
// Works out how far each object is from its own epoch, every frame.
void OrbitPropagationSystem::UpdateTimes(entt::registry& registry, const std::chrono::system_clock::time_point& instant)
{
    if (!m_pRoster)
    {
        return;
    }

    const EntityRoster& entities = *m_pRoster;
    m_Times.clear();
    m_Times.reserve(entities.size());

    for (const EntityHandle entityHandle : entities)
    {
        const OrbitalElementsComponent* pOrbitalElements = registry.try_get<OrbitalElementsComponent>(entityHandle);

        // The roster was built from entities that had one a moment ago, in this same frame, so
        // this cannot miss - but the count has to match the upload exactly or objects would be
        // propagated to each other's times, and a zero is a great deal safer than a shifted array.
        m_Times.push_back(pOrbitalElements ? static_cast<float>(std::chrono::duration<double, std::ratio<60>>(instant - pOrbitalElements->GetEpoch()).count()) : 0.0f);
    }

    m_pComputePass->SetTimes(m_Times, instant);
}

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
        if (!pOrbitalState || !pTransform)
        {
            continue;
        }

        // An element set the propagator could not make sense of - drag has taken its eccentricity
        // out of range, or the orbit has come down - reports as much rather than returning a
        // position. This is not something SGP4Initialise() could have flagged, as these all propagate
        // cleanly at their own epoch.
        const SGP4StepOutput& state = results.states[i];
        if (state.error != static_cast<uint32_t>(SGP4Error::None))
        {
            // The first failure is the one kept: a readback dispatched before the object was
            // retired can land after it and report the same failure again.
            if (!registry.all_of<PropagationFailureComponent>(entityHandle))
            {
                registry.emplace<PropagationFailureComponent>(entityHandle, static_cast<SGP4Error>(state.error), results.time);
            }
            continue;
        }

        const glm::dvec3 position(state.position); // Position is in km, in ECI coordinates
        pTransform->transform = glm::translate(glm::mat4(1.0f), glm::vec3(ECIToWorld(position)));
        UpdateOrbitalState(*pOrbitalState, position, gmst, glm::length(glm::dvec3(state.velocity)));
    }
}

// The readouts that follow from a propagated position, which the web interop reads straight off the component.
void OrbitPropagationSystem::UpdateOrbitalState(OrbitalStateComponent& orbitalState, const glm::dvec3& positionECI, double gmst, double speed)
{
    orbitalState.m_PositionECI = positionECI;
    orbitalState.m_Velocity = speed;

    const glm::dvec3 geodetic = ECIToGeodetic(positionECI, gmst);
    orbitalState.m_Latitude = geodetic.x;
    orbitalState.m_Longitude = geodetic.y;
    orbitalState.m_Altitude = geodetic.z;
}

// Turns every planet mesh so that its prime meridian sits at the current Greenwich Mean Sidereal Time.
void OrbitPropagationSystem::OrientPlanets(entt::registry& registry, double gmst)
{
    const glm::mat4 rotation(CalculatePlanetRotation(gmst));

    auto view = registry.view<const PlanetComponent, TransformComponent>();
    view.each([&rotation](const PlanetComponent&, TransformComponent& transformComponent) {
        transformComponent.transform = rotation;
    });
}

} // namespace WingsOfSteel
