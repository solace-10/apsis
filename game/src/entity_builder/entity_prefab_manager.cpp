/*

#include "entity_builder/entity_prefab_manager.hpp"

#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

#include <pandora.hpp>
#include <scene/components/component_factory.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

void EntityPrefabManager::GetEntityPrefab(const std::string& resourcePath, EntityPrefabManager::OnEntityPrefabLoadedCallback onEntityPrefabLoaded)
{
    auto it = m_EntityPrefabs.find(resourcePath);
    if (it != m_EntityPrefabs.end())
    {
        onEntityPrefabLoaded(it->second.get());
        return;
    }

    using namespace WingsOfSteel::Pandora;
    
    GetResourceSystem()->RequestResource(resourcePath, [this, resourcePath, onEntityPrefabLoaded](ResourceSharedPtr pResource) {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (!pDataStore)
        {
            return;
        }

        auto it = m_EntityPrefabs.find(resourcePath);
        if (it != m_EntityPrefabs.end())
        {
            onEntityPrefabLoaded(it->second.get());
            return;
        }

        const nlohmann::json& jsonData = pDataStore->Data();
        
        auto pEntityPrefab = std::make_unique<EntityPrefab>();
        pEntityPrefab->name = jsonData.value("name", "");

        auto componentsIt = jsonData.find("components");
        if (componentsIt != jsonData.cend() && componentsIt->is_array())
        {
            for (auto componentData : *componentsIt)
            {
                auto typeIt = componentData.find("type");
                if (typeIt != componentData.cend() && typeIt->is_string())
                {
                    const std::string typeName(*typeIt);
                    IComponentUniquePtr pComponent = ComponentFactory::Create(typeName);
                    if (pComponent == nullptr)
                    {
                        Log::Error() << "ComponentFactory couldn't create component type '" << typeName << "' in '" << resourcePath << ".";
                        return;
                    }
                    else
                    {
                        pComponent->Deserialize(componentData);
                        int a = 0;
                    }
                }
                else
                {
                    Log::Error() << "Unable to deserialize component in component array for '" << resourcePath << "', as it has no 'type' attribute.";
                    return;
                }
            }
        }
        
        const std::string typeString = jsonData.value("type", "Ship");
        auto entityType = magic_enum::enum_cast<EntityType>(typeString);
        pEntityPrefab->type = entityType.value_or(EntityType::Ship);
        
        pEntityPrefab->model = jsonData.value("model", "");
        pEntityPrefab->hull = jsonData.value("hull", 0.0f);
        pEntityPrefab->mass = jsonData.value("mass", 0.0f);
        pEntityPrefab->engineForce = jsonData.value("engine_force", 0.0f);
        pEntityPrefab->engineTorque = jsonData.value("engine_torque", 0.0f);
        
        const std::string hardpointsKey("hardpoints");
        if (jsonData.contains(hardpointsKey) && jsonData[hardpointsKey].is_array())
        {
            for (const auto& hardpointJson : jsonData[hardpointsKey])
            {
                HardpointPrefab hardpoint;
                hardpoint.name = hardpointJson.value("name", "");
                hardpoint.minArc = hardpointJson.value("min_arc", 0.0f);
                hardpoint.maxArc = hardpointJson.value("max_arc", 0.0f);
                hardpoint.weapon = hardpointJson.value("weapon", "");
                pEntityPrefab->hardpoints.push_back(hardpoint);
            }
        }
        
        m_EntityPrefabs[resourcePath] = std::move(pEntityPrefab);
        onEntityPrefabLoaded(m_EntityPrefabs[resourcePath].get());
    });
}

} // namespace WingsOfSteel::TheBrightestStar

*/