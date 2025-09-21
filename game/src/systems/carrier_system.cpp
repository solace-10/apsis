#include <scene/components/transform_component.hpp>

#include "components/carrier_component.hpp"
#include "components/faction_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/sector.hpp"
#include "systems/carrier_system.hpp"
#include "game.hpp"

#include <core/log.hpp>

namespace WingsOfSteel
{

void CarrierSystem::Update(float delta)
{
    entt::registry& registry = Game::Get()->GetSector()->GetRegistry();
    auto carrierView = registry.view<const TransformComponent, CarrierComponent>();
    carrierView.each([delta, this](const auto entityHandle, const TransformComponent& transformComponent, CarrierComponent& carrierComponent) {
        carrierComponent.TimeToNextLaunch -= delta;
        if (carrierComponent.TimeToNextLaunch <= 0.0f && !carrierComponent.CurrentLaunch && !carrierComponent.QueuedLaunches.empty())
        {
            carrierComponent.CurrentLaunch = carrierComponent.QueuedLaunches.front();
            carrierComponent.QueuedLaunches.pop_front();

            Sector* pSector = Game::Get()->GetSector();
            SceneWeakPtr pWeakScene = pSector->GetWeakPtr();
            EntitySharedPtr pCarrierEntity = pSector->GetEntity(entityHandle);
            EntityBuilder::Build(pWeakScene, carrierComponent.CurrentLaunch->GetResourcePath(), glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 0.0f, 0.0f)), [pCarrierEntity](EntitySharedPtr pEntity){
                CarrierComponent& carrierComponent = pCarrierEntity->GetComponent<CarrierComponent>();
                if (carrierComponent.CurrentLaunch)
                {
                    pEntity->AddComponent<WingComponent>().ID = carrierComponent.CurrentLaunch->GetWingID();
                    pEntity->AddComponent<FactionComponent>().Value = Faction::Hostile;
                    carrierComponent.CurrentLaunch.reset();
                }
                carrierComponent.TimeToNextLaunch = 2.0f;
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