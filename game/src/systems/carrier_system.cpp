#include <sstream>

#include <glm/gtc/quaternion.hpp>

#include <core/log.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <render/debug_render.hpp>
#include <pandora.hpp>

#include "components/ai_strikecraft_controller_component.hpp"
#include "components/carrier_component.hpp"
#include "components/carrier_launch_component.hpp"
#include "components/faction_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/sector.hpp"
#include "systems/carrier_system.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

void CarrierSystem::Update(float delta)
{
    ProcessLaunchQueues(delta);
    UpdateLaunchSequences(delta);
}

void CarrierSystem::ProcessLaunchQueues(float delta)
{
    Sector* pSector = Game::Get()->GetSector();
    entt::registry& registry = pSector->GetRegistry();
    auto carrierView = registry.view<const TransformComponent, CarrierComponent>();
    carrierView.each([delta, pSector](const auto entityHandle, const TransformComponent& transformComponent, CarrierComponent& carrierComponent) {
        carrierComponent.TimeToNextLaunch -= delta;
        if (carrierComponent.TimeToNextLaunch <= 0.0f && !carrierComponent.CurrentLaunch && !carrierComponent.QueuedLaunches.empty())
        {
            if (carrierComponent.LaunchWaypoints.empty())
            {
                Log::Warning() << "Trying to spawn escorts without launch waypoints.";
                return;
            }

            carrierComponent.CurrentLaunch = carrierComponent.QueuedLaunches.front();
            carrierComponent.QueuedLaunches.pop_front();

            const glm::mat4 carrierWorldTransform(transformComponent.transform);
            const glm::mat4 spawnTransform = carrierWorldTransform * carrierComponent.LaunchWaypoints[0].transform;

            EntitySharedPtr pCarrierEntity = pSector->GetEntity(entityHandle);
            EntityBuilder::Build(carrierComponent.CurrentLaunch->GetResourcePath(), spawnTransform, [pCarrierEntity](EntitySharedPtr pEntity){
                CarrierComponent& carrierComponent = pCarrierEntity->GetComponent<CarrierComponent>();
                if (carrierComponent.CurrentLaunch)
                {
                    WingComponent& wingComponent = pEntity->AddComponent<WingComponent>();
                    wingComponent.ID = carrierComponent.CurrentLaunch->GetWingID();
                    wingComponent.Role = carrierComponent.CurrentLaunch->GetWingRole();
                    carrierComponent.CurrentLaunch.reset();
                }
                carrierComponent.TimeToNextLaunch = carrierComponent.TimeBetweenLaunches;

                CarrierLaunchComponent& carrierLaunchComponent = pEntity->AddComponent<CarrierLaunchComponent>();
                carrierLaunchComponent.LaunchedBy = pCarrierEntity;

                if (pEntity->HasComponent<RigidBodyComponent>())
                {
                    pEntity->GetComponent<RigidBodyComponent>().SetMotionType(MotionType::Kinematic);
                }
                else
                {
                    Log::Error() << "Escort has no RigidBodyComponent.";
                }
            });
        }
    });
}

void CarrierSystem::UpdateLaunchSequences(float delta)
{
    Sector* pSector = Game::Get()->GetSector();
    entt::registry& registry = pSector->GetRegistry();
    auto carrierView = registry.view<const TransformComponent, CarrierComponent, const ModelComponent>();
    carrierView.each([this, delta, pSector](const auto carrierEntityHandle, const TransformComponent& transformComponent, CarrierComponent& carrierComponent, const ModelComponent& modelComponent) {

        if (!carrierComponent.LaunchPointsInitialized)
        {
            InitializeLaunchWaypoints(carrierComponent, modelComponent);
        }

#if DRAW_DEBUG_LAUNCH_WAYPOINTS
        DrawDebugLaunchWaypoints(carrierComponent, transformComponent);
#endif
    });

    auto escortView = pSector->GetRegistry().view<CarrierLaunchComponent>();
    escortView.each([delta, pSector](const auto escortEntityHandle, CarrierLaunchComponent& carrierLaunchComponent) {
        EntitySharedPtr pCarrier = carrierLaunchComponent.LaunchedBy.lock();
        if (!pCarrier)
        {
            return;
        }

        const float launchDuration = 3.0f;
        carrierLaunchComponent.InterpolationValue += delta / launchDuration;
        bool launchFinished = false;
        if (carrierLaunchComponent.InterpolationValue >= 1.0f)
        {
            launchFinished = true;
            carrierLaunchComponent.InterpolationValue = 1.0f;
        }        

        CarrierComponent& carrierComponent = pCarrier->GetComponent<CarrierComponent>();

        // Find the waypoint pair to interpolate between
        const size_t numWaypoints = carrierComponent.LaunchWaypoints.size();
        if (numWaypoints == 0)
        {
            return;
        }

        EntitySharedPtr pEscort = pSector->GetEntity(escortEntityHandle);
        if (!pEscort || !pEscort->HasComponent<TransformComponent>())
        {
            return;
        }

        glm::mat4 interpolatedLocalTransform;

        if (numWaypoints == 1)
        {
            // Only one waypoint, just use it directly
            interpolatedLocalTransform = carrierComponent.LaunchWaypoints[0].transform;
        }
        else
        {
            // Find which waypoint segment we're in
            size_t startWaypointIndex = 0;
            for (size_t i = 0; i < numWaypoints - 1; i++)
            {
                if (carrierLaunchComponent.InterpolationValue >= carrierComponent.LaunchWaypoints[i].ratio &&
                    carrierLaunchComponent.InterpolationValue <= carrierComponent.LaunchWaypoints[i + 1].ratio)
                {
                    startWaypointIndex = i;
                    break;
                }
            }

            const CarrierComponent::LaunchWaypoint& startWaypoint = carrierComponent.LaunchWaypoints[startWaypointIndex];
            const CarrierComponent::LaunchWaypoint& endWaypoint = carrierComponent.LaunchWaypoints[startWaypointIndex + 1];

            // Calculate local interpolation factor between these two waypoints
            float localT = 0.0f;
            float ratioDelta = endWaypoint.ratio - startWaypoint.ratio;
            if (ratioDelta > 0.0f)
            {
                localT = (carrierLaunchComponent.InterpolationValue - startWaypoint.ratio) / ratioDelta;
            }

            // Decompose transforms
            glm::vec3 startPos(startWaypoint.transform[3]);
            glm::vec3 endPos(endWaypoint.transform[3]);
            glm::quat startRot = glm::quat_cast(glm::mat3(startWaypoint.transform));
            glm::quat endRot = glm::quat_cast(glm::mat3(endWaypoint.transform));

            // Interpolate position and rotation
            glm::vec3 interpolatedPos = glm::mix(startPos, endPos, localT);
            glm::quat interpolatedRot = glm::slerp(startRot, endRot, localT);

            // Reconstruct transform
            interpolatedLocalTransform = glm::mat4_cast(interpolatedRot);
            interpolatedLocalTransform[3] = glm::vec4(interpolatedPos, 1.0f);
        }

        // Transform from carrier local space to world space
        const glm::mat4& carrierWorldTransform = pCarrier->GetComponent<TransformComponent>().transform;
        pEscort->GetComponent<RigidBodyComponent>().SetWorldTransform(carrierWorldTransform * interpolatedLocalTransform);

        // When launch is finished, enable physics
        if (launchFinished)
        {
            glm::mat4 finalTransform = carrierWorldTransform * interpolatedLocalTransform;
            finalTransform[3].y = 0.0f;
            pEscort->GetComponent<RigidBodyComponent>().SetWorldTransform(finalTransform);

            if (pEscort->HasComponent<RigidBodyComponent>())
            {
                pEscort->GetComponent<RigidBodyComponent>().SetMotionType(MotionType::Dynamic);
            }

            pEscort->RemoveComponent<CarrierLaunchComponent>();

            if (pEscort->HasComponent<AIStrikecraftControllerComponent>())
            {
                pEscort->GetComponent<AIStrikecraftControllerComponent>().SetState(AIStrikecraftState::Approach);
            }
        }

    });
}

void CarrierSystem::LaunchEscorts(EntitySharedPtr pCarrierEntity, const std::vector<std::string>& escorts, WingRole wingRole)
{
    if (!pCarrierEntity->HasComponent<CarrierComponent>())
    {
        Log::Error() << "Trying to launch escorts from an entity which doesn't have a CarrierComponent.";
        return;
    }

    CarrierComponent& carrierComponent = pCarrierEntity->GetComponent<CarrierComponent>();
    for (const std::string& resourcePath : escorts)
    {
        carrierComponent.QueuedLaunches.emplace_back(resourcePath, carrierComponent.CurrentWingID, wingRole);
    }

    carrierComponent.CurrentWingID++;
}

void CarrierSystem::InitializeLaunchWaypoints(CarrierComponent& carrierComponent, const ModelComponent& modelComponent)
{
    // Check if the model is available. It should be, but there might be edge cases where the resource isn't available yet.
    ResourceModelSharedPtr pModel = modelComponent.GetModel();
    if (!pModel)
    {
        return;
    }

    for (int waypointIndex = 1; waypointIndex < 10; waypointIndex++)
    {
        std::stringstream waypointName;
        waypointName << "BayWaypoint" << waypointIndex;

        std::optional<ResourceModel::AttachmentPoint> waypoint = pModel->GetAttachmentPoint(waypointName.str());
        if (waypoint.has_value())
        {
            CarrierComponent::LaunchWaypoint launchWaypoint
            {
                .transform = waypoint.value().m_ModelTransform,
                .ratio = 0.0f
            };
            carrierComponent.LaunchWaypoints.push_back(launchWaypoint);
        }
        else
        {
            break;
        }
    }

    const size_t numWaypoints = carrierComponent.LaunchWaypoints.size();
    if (numWaypoints == 0)
    {
        Log::Warning() << pModel->GetPath() << ": No launch waypoints found for carrier model (should be called BayWaypoint#).";
    }
    else
    {
        // Calculate total distance between the start and the end of the launch.
        float totalDistance = 0.0f;
        const size_t numWaypoints = carrierComponent.LaunchWaypoints.size();
        for (size_t waypointIndex = 0; waypointIndex < numWaypoints - 1; waypointIndex++)
        {
            const glm::vec3 currentWaypointPosition(carrierComponent.LaunchWaypoints[waypointIndex].transform[3]);
            const glm::vec3 nextWaypointPosition(carrierComponent.LaunchWaypoints[waypointIndex + 1].transform[3]);
            totalDistance += glm::distance(currentWaypointPosition, nextWaypointPosition);
        }

        if (numWaypoints == 1)
        {
            carrierComponent.LaunchWaypoints[0].ratio = 1.0f;
        }
        else
        {
            // Calculate the distance ratio for each waypoint. 
            // The first waypoint is 0, the last waypoint is 1.
            // This will allow the escorts' transforms to be interpolated along the path in UpdateLaunchSequences().
            float accumulatedDistance = 0.0f;
            carrierComponent.LaunchWaypoints[0].ratio = 0.0f;
            for (size_t waypointIndex = 1; waypointIndex < numWaypoints; waypointIndex++)
            {
                const glm::vec3 previousWaypointPosition(carrierComponent.LaunchWaypoints[waypointIndex - 1].transform[3]);
                const glm::vec3 currentWaypointPosition(carrierComponent.LaunchWaypoints[waypointIndex].transform[3]);
                accumulatedDistance += glm::distance(previousWaypointPosition, currentWaypointPosition);
                carrierComponent.LaunchWaypoints[waypointIndex].ratio = accumulatedDistance / totalDistance;
            }
        }
    }

    carrierComponent.LaunchPointsInitialized = true;
}

#if DRAW_DEBUG_LAUNCH_WAYPOINTS
void CarrierSystem::DrawDebugLaunchWaypoints(const CarrierComponent& carrierComponent, const TransformComponent& transformComponent)
{
    const size_t numWaypoints = carrierComponent.LaunchWaypoints.size();
        for (size_t waypointIndex = 0; waypointIndex < numWaypoints; waypointIndex++)
        {
            const glm::mat4 waypointTransform = transformComponent.transform * carrierComponent.LaunchWaypoints[waypointIndex].transform;
            const glm::vec3 waypointPosition(waypointTransform[3]);
            GetDebugRender()->Box(waypointPosition, Color::Cyan, 1.5f, 1.5f, 1.5f);

            if (waypointIndex + 1 < numWaypoints)
            {
                const glm::mat4 nextWaypointTransform = transformComponent.transform * carrierComponent.LaunchWaypoints[waypointIndex + 1].transform;
                const glm::vec3 nextWaypointPosition(nextWaypointTransform[3]);
                GetDebugRender()->Arrow(waypointPosition, nextWaypointPosition, Color::DarkCyan, 1.0f);
            }
        }
}
#endif

} // namespace WingsOfSteel