#include "ship_builder/ship_builder.hpp"

#include <core/log.hpp>
#include <pandora.hpp>
#include <resources/resource_model.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/hardpoint_component.hpp"
#include "components/name_component.hpp"
#include "components/ship_engine_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/weapon_component.hpp"
#include "game.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

void ShipBuilder::Build(Pandora::EntitySharedPtr pShip, const glm::mat4& worldTransform)
{
    using namespace Pandora;

    GetResourceSystem()->RequestResource("/models/player/destroyer.glb", [pShip, worldTransform](ResourceSharedPtr pResource) {
        ResourceModelSharedPtr pResourceModel = std::dynamic_pointer_cast<ResourceModel>(pResource);
        TransformComponent& transformComponent = pShip->AddComponent<TransformComponent>();
        // transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        transformComponent.transform = worldTransform;

        pShip->AddComponent<ModelComponent>(pResourceModel);
        pShip->AddComponent<ShipNavigationComponent>();
        pShip->AddComponent<NameComponent>("Everflame");

        ShipEngineComponent& engineComponent = pShip->AddComponent<ShipEngineComponent>();
        engineComponent.linearForce = 1300.0f;
        engineComponent.torque = 7500.0f;

        RigidBodyConstructionInfo rigidBodyConstructionInfo;
        rigidBodyConstructionInfo.SetWorldTransform(worldTransform);
        rigidBodyConstructionInfo.SetShape(pResourceModel->GetCollisionShape());
        rigidBodyConstructionInfo.SetMass(100);
        rigidBodyConstructionInfo.SetLinearDamping(0.5f);
        rigidBodyConstructionInfo.SetAngularDamping(0.5f);

        pShip->AddComponent<RigidBodyComponent>(rigidBodyConstructionInfo);

        const std::string turretPortName = "TurretPort";
        if (AddHardpoint(pShip, turretPortName, -5.0f, 120.0f))
        {
            AddWeapon(pShip, turretPortName, "Weapon1");
        }

        const std::string turretStarboardName = "TurretStarboard";
        if (AddHardpoint(pShip, turretStarboardName, -120.0f, 5.0f))
        {
            AddWeapon(pShip, turretStarboardName, "Weapon1");
        }
    });
}

bool ShipBuilder::AddHardpoint(Pandora::EntitySharedPtr pShip, const std::string& attachmentPointName, float minArc, float maxArc)
{
    using namespace Pandora;

    // Ensure the ship has a HardpointComponent. We can only have one HardpointComponent, but will
    // likely call addHardpoint() multiple times.
    HardpointComponent* pHardpointComponent = nullptr;
    if (pShip->HasComponent<HardpointComponent>())
    {
        pHardpointComponent = &pShip->GetComponent<HardpointComponent>();
    }
    else
    {
        pHardpointComponent = &pShip->AddComponent<HardpointComponent>();
    }

    ModelComponent& modelComponent = pShip->GetComponent<ModelComponent>();
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
    hp.m_pParent = pShip;
    pHardpointComponent->hardpoints.push_back(hp);
    return true;
}

Pandora::Result<ShipBuilder::EAddWeaponResult> ShipBuilder::AddWeapon(Pandora::EntitySharedPtr pShip, const std::string& hardpointName, const std::string& weaponName)
{
    using namespace Pandora;

    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return Result<EAddWeaponResult>(EAddWeaponResult::InvalidSector);
    }

    Hardpoint hardpoint;
    if (!FindHardpoint(pShip, hardpointName, hardpoint))
    {
        return Result<EAddWeaponResult>(EAddWeaponResult::InvalidHardpoint);
    }

    EntitySharedPtr pWeaponEntity = pSector->CreateEntity();
    TransformComponent& transformComponent = pWeaponEntity->AddComponent<TransformComponent>();
    WeaponComponent& weaponComponent = pWeaponEntity->AddComponent<WeaponComponent>();
    weaponComponent.m_pParent = pShip;
    weaponComponent.m_ArcMinDegrees = hardpoint.m_ArcMinDegrees;
    weaponComponent.m_ArcMaxDegrees = hardpoint.m_ArcMaxDegrees;
    weaponComponent.m_Range = 100.0f;
    weaponComponent.m_AttachmentPointTransform = hardpoint.m_AttachmentPointTransform;

    GetResourceSystem()->RequestResource("/models/weapons/heavy_mass_driver.glb", [pWeaponEntityWeak = std::weak_ptr<Entity>(pWeaponEntity)](ResourceSharedPtr pResource) {
        ResourceModelSharedPtr pResourceModel = std::dynamic_pointer_cast<ResourceModel>(pResource);
        if (auto pWeaponEntity = pWeaponEntityWeak.lock())
        {
            pWeaponEntity->AddComponent<ModelComponent>(pResourceModel);
        }
    });

    return Pandora::Result<EAddWeaponResult>(EAddWeaponResult::Ok);
}

bool ShipBuilder::FindHardpoint(Pandora::EntitySharedPtr pShip, const std::string& hardpointName, Hardpoint& outHardpoint)
{
    using namespace Pandora;
    if (!pShip->HasComponent<HardpointComponent>())
    {
        Log::Error() << "Attempting to find a hardpoint on a ship without a hardpoint component.";
        return false;
    }

    HardpointComponent& hardpointComponent = pShip->GetComponent<HardpointComponent>();
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
