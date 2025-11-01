#include "entity_builder/entity_builder.hpp"

#include <vector>

#include <core/log.hpp>
#include <pandora.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/component_factory.hpp>
#include <scene/components/entity_reference_component.hpp>
#include <scene/components/ghost_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/scene.hpp>

#include "components/hardpoint_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "systems/weapon_system.hpp"

namespace WingsOfSteel
{

void EntityBuilder::Build(SceneWeakPtr& pWeakScene, const std::string& prefabResourcePath, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback)
{
    GetResourceSystem()->RequestResource(prefabResourcePath, [pWeakScene, prefabResourcePath, worldTransform, onEntityReadyCallback](ResourceSharedPtr pResource)
    {
        SceneSharedPtr pScene = pWeakScene.lock();
        if (pScene == nullptr)
        {
            return;
        }
        EntitySharedPtr pEntity = pScene->CreateEntity();

        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        const Json::Data& jsonData = pDataStore->Data();

        std::vector<std::string> resourcesToLoad;
        auto resourcesIt = jsonData.find("resources");
        if (resourcesIt != jsonData.cend() && resourcesIt->is_array())
        {
            for (auto resourceData : *resourcesIt)
            {
                if (resourceData.is_string())
                {
                    resourcesToLoad.push_back(resourceData.get<std::string>());
                }
            }
        }

        if (resourcesToLoad.empty())
        {
            InstantiateComponents(pEntity, pDataStore.get(), jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath);
        }
        else
        {
            GetResourceSystem()->RequestResources(resourcesToLoad, [pEntity, pDataStore, jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath](std::unordered_map<std::string, ResourceSharedPtr>)
            {
                InstantiateComponents(pEntity, pDataStore.get(), jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath);
            });
        }
    });
}

void EntityBuilder::Build(const std::string& prefabResourcePath, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback)
{
    SceneWeakPtr pScene = Game::Get()->GetSector()->GetWeakPtr();
    Build(pScene, prefabResourcePath, worldTransform, onEntityReadyCallback);
}

void EntityBuilder::InstantiateComponents(EntitySharedPtr pEntity, const ResourceDataStore* pContext, const Json::Data& jsonData, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback, const std::string& prefabResourcePath)
{
    auto componentsIt = jsonData.find("components");
    if (componentsIt != jsonData.cend() && componentsIt->is_array())
    {
        for (auto componentData : *componentsIt)
        {
            auto typeIt = componentData.find("type");
            if (typeIt != componentData.cend() && typeIt->is_string())
            {
                const std::string typeName(*typeIt);

                if (!ComponentFactory::Create(pEntity.get(), pContext, typeName, componentData))
                {
                    Log::Error() << "Don't know how to create component type '" << typeName << "' in '" << prefabResourcePath << "'.";
                    return;
                }
            }
            else
            {
                Log::Error() << "Unable to deserialize component in component array for '" << prefabResourcePath << "', as it has no 'type' attribute.";
                return;
            }
        }
    }

    if (pEntity->HasComponent<TransformComponent>())
    {
        pEntity->GetComponent<TransformComponent>().transform = worldTransform;
    }
    else
    {
        pEntity->AddComponent<TransformComponent>().transform = worldTransform;
    }

    if (pEntity->HasComponent<RigidBodyComponent>())
    {
        RigidBodyComponent& rigidBodyComponent = pEntity->GetComponent<RigidBodyComponent>();
        rigidBodyComponent.SetOwner(pEntity);
        rigidBodyComponent.SetWorldTransform(worldTransform);
    }

    if (pEntity->HasComponent<GhostComponent>())
    {
        GhostComponent& ghostComponent = pEntity->GetComponent<GhostComponent>();
        ghostComponent.SetOwner(pEntity);
        ghostComponent.SetWorldTransform(worldTransform);
    }

    if (pEntity->HasComponent<EntityReferenceComponent>())
    {
        pEntity->GetComponent<EntityReferenceComponent>().SetOwner(pEntity);
    }

    if (pEntity->HasComponent<HardpointComponent>())
    {
        const auto& hardpointComponent = pEntity->GetComponent<HardpointComponent>();
        for (const auto& hardpoint : hardpointComponent.hardpoints)
        {
            if (!hardpoint.m_Weapon.empty())
            {
                Game::Get()->GetSector()->GetSystem<WeaponSystem>()->AttachWeapon(
                    hardpoint.m_Weapon,
                    pEntity,
                    hardpoint.m_Name,
                    hardpoint.m_AutomatedTargeting
                );
            }
        }
    }

    if (onEntityReadyCallback)
    {
        onEntityReadyCallback(pEntity);
    }
}

} // namespace WingsOfSteel
