#include <core/log.hpp>
#include <scene/components/transform_component.hpp>

#include "components/carrier_component.hpp"
#include "components/faction_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/sector.hpp"
#include "systems/carrier_system.hpp"
#include "game.hpp"

namespace WingsOfSteel
{

void CarrierSystem::Update(float delta)
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
                    pEntity->AddComponent<WingComponent>().ID = carrierComponent.CurrentLaunch->GetWingID();
                    pEntity->AddComponent<FactionComponent>().Value = Faction::Hostile;
                    carrierComponent.CurrentLaunch.reset();
                }
                carrierComponent.TimeToNextLaunch = carrierComponent.TimeBetweenLaunches;
            });
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

} // namespace WingsOfSteel