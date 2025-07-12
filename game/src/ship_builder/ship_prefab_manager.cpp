#include "ship_builder/ship_prefab_manager.hpp"

#include <nlohmann/json.hpp>

#include <pandora.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

void ShipPrefabManager::GetShipPrefab(const std::string& resourcePath, ShipPrefabManager::OnShipPrefabLoadedCallback onShipPrefabLoaded)
{
    auto it = m_ShipPrefabs.find(resourcePath);
    if (it != m_ShipPrefabs.end())
    {
        onShipPrefabLoaded(it->second.get());
        return;
    }

    using namespace WingsOfSteel::Pandora;
    
    GetResourceSystem()->RequestResource(resourcePath, [this, resourcePath, onShipPrefabLoaded](ResourceSharedPtr pResource) {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (!pDataStore)
        {
            return;
        }

        auto it = m_ShipPrefabs.find(resourcePath);
        if (it != m_ShipPrefabs.end())
        {
            onShipPrefabLoaded(it->second.get());
            return;
        }

        const nlohmann::json& jsonData = pDataStore->Data();
        
        auto pShipPrefab = std::make_unique<ShipPrefab>();
        pShipPrefab->name = jsonData.value("name", "");
        pShipPrefab->model = jsonData.value("model", "");
        pShipPrefab->hull = jsonData.value("hull", 0.0f);
        pShipPrefab->mass = jsonData.value("mass", 0.0f);
        pShipPrefab->engineForce = jsonData.value("engine_force", 0.0f);
        pShipPrefab->engineTorque = jsonData.value("engine_torque", 0.0f);
        
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
                pShipPrefab->hardpoints.push_back(hardpoint);
            }
        }
        
        m_ShipPrefabs[resourcePath] = std::move(pShipPrefab);
        onShipPrefabLoaded(m_ShipPrefabs[resourcePath].get());
    });
}

} // namespace WingsOfSteel::TheBrightestStar
