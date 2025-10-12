#include <sstream>

#include <core/log.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <render/debug_render.hpp>
#include <pandora.hpp>

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
            carrierComponent.CurrentLaunch = carrierComponent.QueuedLaunches.front();
            carrierComponent.QueuedLaunches.pop_front();

            const glm::mat4 spawnTransform = glm::translate(glm::mat4(1.0f), transformComponent.GetForward() * -50.0f) * transformComponent.transform;
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

        DrawDebugLaunchWaypoints(carrierComponent, transformComponent);

        EntitySharedPtr pCarrierEntity = pSector->GetEntity(carrierEntityHandle);

        auto escortView = pSector->GetRegistry().view<CarrierLaunchComponent>();
        escortView.each([delta, pSector](const auto escortEntityHandle, CarrierLaunchComponent carrierLaunchComponent) {

        });
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
            carrierComponent.LaunchWaypoints.push_back(waypoint.value().m_ModelTransform);
        }
        else
        {
            break;
        }
    }

    if (carrierComponent.LaunchWaypoints.empty())
    {
        Log::Warning() << pModel->GetPath() << ": No launch waypoints found for carrier model (should be called BayWaypoint#).";
    }

    carrierComponent.LaunchPointsInitialized = true;
}

void CarrierSystem::DrawDebugLaunchWaypoints(const CarrierComponent& carrierComponent, const TransformComponent& transformComponent)
{
    const size_t numWaypoints = carrierComponent.LaunchWaypoints.size();
        for (size_t waypointIndex = 0; waypointIndex < numWaypoints; waypointIndex++)
        {
            const glm::mat4 waypointTransform = transformComponent.transform * carrierComponent.LaunchWaypoints[waypointIndex];
            const glm::vec3 waypointPosition(waypointTransform[3]);
            GetDebugRender()->Box(waypointPosition, Color::Cyan, 1.5f, 1.5f, 1.5f);

            if (waypointIndex + 1 < numWaypoints)
            {
                const glm::mat4 nextWaypointTransform = transformComponent.transform * carrierComponent.LaunchWaypoints[waypointIndex + 1];
                const glm::vec3 nextWaypointPosition(nextWaypointTransform[3]);
                GetDebugRender()->Arrow(waypointPosition, nextWaypointPosition, Color::DarkCyan, 1.0f);
            }
        }
}

} // namespace WingsOfSteel