#include "entity_builder/entity_builder.hpp"

#include <core/log.hpp>
#include <pandora.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_model.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/component_factory.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/hardpoint_component.hpp"
#include "components/weapon_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"
#include "entity_builder/entity_prefab_manager.hpp"

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
        
        auto componentsIt = jsonData.find("components");
        if (componentsIt != jsonData.cend() && componentsIt->is_array())
        {
            for (auto componentData : *componentsIt)
            {
                auto typeIt = componentData.find("type");
                if (typeIt != componentData.cend() && typeIt->is_string())
                {
                    const std::string typeName(*typeIt);

                    if (!ComponentFactory::CreateAndAddToEntity(pEntity.get(), typeName, componentData))
                    {
                        Log::Error() << "Don't know how to create component type '" << typeName << "' in '" << prefabResourcePath << "'.";
                    }
                }
                else
                {
                    Log::Error() << "Unable to deserialize component in component array for '" << prefabResourcePath << "', as it has no 'type' attribute.";
                    return;
                }
            }
        }

        TransformComponent transformComponent = pEntity->AddComponent<TransformComponent>();
        transformComponent.transform = worldTransform;

        if (onEntityReadyCallback)
        {
            onEntityReadyCallback(pEntity);
        }
    });

    /*
    Game::Get()->GetEntityPrefabManager()->GetEntityPrefab(prefabResourcePath, [pEntity, worldTransform](const EntityPrefabManager::EntityPrefab* pEntityPrefab) {
        if (!pEntityPrefab)
        {
            Log::Error() << "Failed to load entity prefab";
            return;
        }



        GetResourceSystem()->RequestResource(pEntityPrefab->model, [pEntity, worldTransform, pEntityPrefab](ResourceSharedPtr pResource) {
            ResourceModelSharedPtr pResourceModel = std::dynamic_pointer_cast<ResourceModel>(pResource);
            TransformComponent& transformComponent = pEntity->AddComponent<TransformComponent>();
            transformComponent.transform = worldTransform;

            pEntity->AddComponent<ModelComponent>(pResourceModel);
            pEntity->AddComponent<NameComponent>(pEntityPrefab->name);

            RigidBodyConstructionInfo rigidBodyConstructionInfo;
            rigidBodyConstructionInfo.SetWorldTransform(worldTransform);
            rigidBodyConstructionInfo.SetShape(pResourceModel->GetCollisionShape());
            rigidBodyConstructionInfo.SetMass(pEntityPrefab->mass);
            rigidBodyConstructionInfo.SetLinearDamping(0.5f);
            rigidBodyConstructionInfo.SetAngularDamping(0.5f);

            auto& rbc = pEntity->AddComponent<RigidBodyComponent>();
            rbc.Initialize(rigidBodyConstructionInfo);

            if (pEntityPrefab->type == EntityPrefabManager::EntityType::Ship)
            {
                ShipEngineComponent& engineComponent = pEntity->AddComponent<ShipEngineComponent>();
                engineComponent.linearForce = pEntityPrefab->engineForce;
                engineComponent.torque = pEntityPrefab->engineTorque;

                pEntity->AddComponent<ShipNavigationComponent>();
            }
            else if (pEntityPrefab->type == EntityPrefabManager::EntityType::Mech)
            {
                MechEngineComponent& engineComponent = pEntity->AddComponent<MechEngineComponent>();
                engineComponent.linearForce = pEntityPrefab->engineForce;
                engineComponent.torque = pEntityPrefab->engineTorque;

                pEntity->AddComponent<MechNavigationComponent>();
            }

            for (const auto& hardpoint : pEntityPrefab->hardpoints)
            {
                if (AddHardpoint(pEntity, hardpoint.name, hardpoint.minArc, hardpoint.maxArc))
                {
                    if (!hardpoint.weapon.empty())
                    {
                        AddWeapon(pEntity, hardpoint.name, hardpoint.weapon);
                    }
                }
            }
        });
    });
    */
}

bool EntityBuilder::AddHardpoint(Pandora::EntitySharedPtr pEntity, const std::string& attachmentPointName, float minArc, float maxArc)
{
    using namespace Pandora;

    // Ensure the ship has a HardpointComponent. We can only have one HardpointComponent, but will
    // likely call addHardpoint() multiple times.
    HardpointComponent* pHardpointComponent = nullptr;
    if (pEntity->HasComponent<HardpointComponent>())
    {
        pHardpointComponent = &pEntity->GetComponent<HardpointComponent>();
    }
    else
    {
        pHardpointComponent = &pEntity->AddComponent<HardpointComponent>();
    }

    ModelComponent& modelComponent = pEntity->GetComponent<ModelComponent>();
    ResourceModelSharedPtr pModel = modelComponent.GetModel();
    if (!pModel)
    {
        Log::Error() << "Attempting to add a hardpoint before the parent's model has been loaded.";
        return false;
    }

    std::optional<ResourceModel::AttachmentPoint> pAttachmentPoint = pModel->GetAttachmentPoint(attachmentPointName);
    if (!pAttachmentPoint)
    {
        Log::Error() << "Attachment point not found: " << attachmentPointName;
        return false;
    }

    Hardpoint hp;
    hp.m_Name = attachmentPointName;
    hp.m_AttachmentPointTransform = pAttachmentPoint->m_ModelTransform;
    hp.m_ArcMinDegrees = minArc;
    hp.m_ArcMaxDegrees = maxArc;
    hp.m_pParent = pEntity;
    pHardpointComponent->hardpoints.push_back(hp);
    return true;
}

Pandora::Result<EntityBuilder::EAddWeaponResult> EntityBuilder::AddWeapon(Pandora::EntitySharedPtr pEntity, const std::string& hardpointName, const std::string& weaponName)
{
    using namespace Pandora;

    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return Result<EAddWeaponResult>(EAddWeaponResult::InvalidSector);
    }

    Hardpoint hardpoint;
    if (!FindHardpoint(pEntity, hardpointName, hardpoint))
    {
        return Result<EAddWeaponResult>(EAddWeaponResult::InvalidHardpoint);
    }

    EntitySharedPtr pWeaponEntity = pSector->CreateEntity();
    TransformComponent& transformComponent = pWeaponEntity->AddComponent<TransformComponent>();
    WeaponComponent& weaponComponent = pWeaponEntity->AddComponent<WeaponComponent>();
    weaponComponent.m_pParent = pEntity;
    weaponComponent.m_ArcMinDegrees = hardpoint.m_ArcMinDegrees;
    weaponComponent.m_ArcMaxDegrees = hardpoint.m_ArcMaxDegrees;
    weaponComponent.m_Range = 100.0f;
    weaponComponent.m_AttachmentPointTransform = hardpoint.m_AttachmentPointTransform;

    GetResourceSystem()->RequestResource("/models/weapons/heavy_mass_driver.glb", [pWeaponEntityWeak = std::weak_ptr<Entity>(pWeaponEntity)](ResourceSharedPtr pResource) {
        ResourceModelSharedPtr pResourceModel = std::dynamic_pointer_cast<ResourceModel>(pResource);
        if (auto pWeaponEntity = pWeaponEntityWeak.lock())
        {
            //pWeaponEntity->AddComponent<ModelComponent>(pResourceModel);
        }
    });

    return Pandora::Result<EAddWeaponResult>(EAddWeaponResult::Ok);
}

bool EntityBuilder::FindHardpoint(Pandora::EntitySharedPtr pEntity, const std::string& hardpointName, Hardpoint& outHardpoint)
{
    using namespace Pandora;
    if (!pEntity->HasComponent<HardpointComponent>())
    {
        Log::Error() << "Attempting to find a hardpoint on a ship without a hardpoint component.";
        return false;
    }

    HardpointComponent& hardpointComponent = pEntity->GetComponent<HardpointComponent>();
    for (const Hardpoint& hardpoint : hardpointComponent.hardpoints)
    {
        if (hardpoint.m_Name == hardpointName)
        {
            outHardpoint = hardpoint;
            return true;
        }
    }

    return false;
}

} // namespace WingsOfSteel::TheBrightestStar
