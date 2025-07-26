#include "entity_builder/entity_builder.hpp"

#include <vector>

#include <core/log.hpp>
#include <pandora.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/component_factory.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/scene.hpp>

#include "game.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void EntityBuilder::Build(Pandora::SceneWeakPtr& pWeakScene, const std::string& prefabResourcePath, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback)
{
    using namespace Pandora;

    GetResourceSystem()->RequestResource(prefabResourcePath, [pWeakScene, prefabResourcePath, worldTransform, onEntityReadyCallback](Pandora::ResourceSharedPtr pResource)
    {
        SceneSharedPtr pScene = pWeakScene.lock();
        if (pScene == nullptr)
        {
            return;
        }
        EntitySharedPtr pEntity = pScene->CreateEntity();

        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        const nlohmann::json& jsonData = pDataStore->Data();

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
            InstantiateComponents(pEntity, jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath);
        }
        else
        {
            GetResourceSystem()->RequestResources(resourcesToLoad, [pEntity, jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath](std::unordered_map<std::string, ResourceSharedPtr>)
            {
                InstantiateComponents(pEntity, jsonData, worldTransform, onEntityReadyCallback, prefabResourcePath);
            });
        }
    });
}

void EntityBuilder::InstantiateComponents(Pandora::EntitySharedPtr pEntity, const nlohmann::json& jsonData, const glm::mat4& worldTransform, OnEntityReady onEntityReadyCallback, const std::string& prefabResourcePath)
{
    using namespace Pandora;
    auto componentsIt = jsonData.find("components");
    if (componentsIt != jsonData.cend() && componentsIt->is_array())
    {
        for (auto componentData : *componentsIt)
        {
            auto typeIt = componentData.find("type");
            if (typeIt != componentData.cend() && typeIt->is_string())
            {
                const std::string typeName(*typeIt);

                if (!ComponentFactory::Create(pEntity.get(), typeName, componentData))
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
        pEntity->GetComponent<RigidBodyComponent>().SetWorldTransform(worldTransform);
    }

    if (onEntityReadyCallback)
    {
        onEntityReadyCallback(pEntity);
    }
}

} // namespace WingsOfSteel::TheBrightestStar
