#include <imgui.h>

#include <core/log.hpp>
#include <pandora.hpp>
#include <physics/collision_shape.hpp>
#include <render/debug_render.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/hardpoint_component.hpp"
#include "components/name_component.hpp"
#include "components/player_controller_component.hpp"
#include "components/sector_camera_component.hpp"
#include "components/ship_engine_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "fleet.hpp"
#include "sector/sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/hardpoint_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Sector::Sector()
{
}

Sector::~Sector()
{
}

void Sector::Initialize()
{
    using namespace Pandora;

    Scene::Initialize();

    AddSystem<ModelRenderSystem>();
    AddSystem<PhysicsSimulationSystem>();
    AddSystem<PlayerControllerSystem>();
    AddSystem<ShipNavigationSystem>();
    AddSystem<HardpointSystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->AddComponent<SectorCameraComponent>();
    sectorCameraComponent.position = glm::vec3(0.0f, 200.0f, 75.0f);
    sectorCameraComponent.target = glm::vec3(0.0f, 0.0f, 0.0f);
    sectorCameraComponent.maximumDrift = glm::vec3(0.0f, 0.0f, 0.0f);
    SetCamera(m_pCamera);

    SpawnDome();
    SpawnPlayerFleet();
    sectorCameraComponent.anchorEntity = m_pPlayerShip;
}

void Sector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    if (m_ShowGrid)
    {
        Pandora::GetDebugRender()->AxisTriad(glm::mat4(1.0f), 10.0f, 50.0f);
        Pandora::GetDebugRender()->XZSquareGrid(-1000.0f, 1000.0f, -1.0f, 100.0f, Pandora::Color::White);
    }

    using namespace Pandora;
    TransformComponent& transformComponent = m_pDome->GetComponent<TransformComponent>();
    transformComponent.transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    DrawCameraDebugUI();
}

void Sector::ShowCameraDebugUI(bool state)
{
    m_ShowCameraDebugUI = state;
}

void Sector::ShowGrid(bool state)
{
    m_ShowGrid = state;
}

void Sector::DrawCameraDebugUI()
{
    if (!m_ShowCameraDebugUI)
    {
        return;
    }

    ImGui::Begin("Camera", &m_ShowCameraDebugUI);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->GetComponent<SectorCameraComponent>();

    ImGui::Checkbox("Debug Draw", &sectorCameraComponent.debugDraw);

    const glm::vec3& position = sectorCameraComponent.position;
    float fposition[3] = { position.x, position.y, position.z };
    if (ImGui::InputFloat3("Eye", fposition))
    {
        sectorCameraComponent.position = glm::vec3(fposition[0], fposition[1], fposition[2]);
    }

    const glm::vec3& drift = sectorCameraComponent.maximumDrift;
    float fdrift[3] = { drift.x, drift.y, drift.z };
    if (ImGui::InputFloat3("Drift", fdrift))
    {
        sectorCameraComponent.maximumDrift = glm::vec3(fdrift[0], fdrift[1], fdrift[2]);
    }

    ImGui::End();
}

void Sector::SpawnDome()
{
    using namespace Pandora;

    m_pDome = CreateEntity();

    SectorCameraComponent& sectorCameraComponent = m_pCamera->GetComponent<SectorCameraComponent>();

    TransformComponent& transformComponent = m_pDome->AddComponent<TransformComponent>();
    // transformComponent.transform = glm::scale(glm::translate(glm::mat4(1.0f), sectorCameraComponent.position + glm::vec3(0.0f, 0.0f, 100.0f)), glm::vec3(20.0f));

    static float rotation = 90.0f;
    transformComponent.transform = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(1.0f, 0.0f, 0.0f));

    GetResourceSystem()->RequestResource("/models/dome/dome.glb", [this](ResourceSharedPtr pResource) {
        m_pDome->AddComponent<ModelComponent>(std::dynamic_pointer_cast<ResourceModel>(pResource));
    });
}

void Sector::SpawnPlayerFleet()
{
    using namespace Pandora;

    m_pPlayerFleet = std::make_shared<Fleet>();

    m_pPlayerShip = CreateEntity();

    GetResourceSystem()->RequestResource("/models/player/destroyer.glb", [this](ResourceSharedPtr pResource) {
        ResourceModelSharedPtr pResourceModel = std::dynamic_pointer_cast<ResourceModel>(pResource);
        TransformComponent& transformComponent = m_pPlayerShip->AddComponent<TransformComponent>();
        transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

        m_pPlayerShip->AddComponent<ModelComponent>(pResourceModel);
        m_pPlayerShip->AddComponent<ShipNavigationComponent>();
        m_pPlayerShip->AddComponent<NameComponent>("Everflame");
        m_pPlayerShip->AddComponent<PlayerControllerComponent>();

        ShipEngineComponent& engineComponent = m_pPlayerShip->AddComponent<ShipEngineComponent>();
        engineComponent.linearForce = 1300.0f;
        engineComponent.torque = 7500.0f;

        RigidBodyConstructionInfo rigidBodyConstructionInfo;
        rigidBodyConstructionInfo.SetShape(pResourceModel->GetCollisionShape());
        rigidBodyConstructionInfo.SetMass(100);
        rigidBodyConstructionInfo.SetLinearDamping(0.5f);
        rigidBodyConstructionInfo.SetAngularDamping(0.5f);

        m_pPlayerShip->AddComponent<RigidBodyComponent>(rigidBodyConstructionInfo);

        auto addHardpoint = [](EntitySharedPtr pShip, const std::string& attachmentPointName, float minArc, float maxArc) {
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
            ResourceModel* pModel = modelComponent.GetModel();
            if (!pModel)
            {
                Log::Error() << "Attempting to add a hardpoint before the parent's model has been loaded.";
                return;
            }

            std::optional<ResourceModel::AttachmentPoint> pAttachmentPoint = pModel->GetAttachmentPoint(attachmentPointName);
            if (!pAttachmentPoint)
            {
                Log::Error() << "Attachment point not found: " << attachmentPointName;
                return;
            }

            Hardpoint hp;
            hp.m_AttachmentPointTransform = pAttachmentPoint->m_ModelTransform;
            hp.m_ArcMinDegrees = minArc;
            hp.m_ArcMaxDegrees = maxArc;
            hp.m_pParent = pShip;
            pHardpointComponent->hardpoints.push_back(hp);
        };

        addHardpoint(m_pPlayerShip, "TurretPort", -5.0f, 120.0f);
        addHardpoint(m_pPlayerShip, "TurretStarboard", -120.0f, 5.0f);
    });

    m_pPlayerFleet->AddShip(m_pPlayerShip);

    // std::array<std::string, 2> escortNames = { "Skyforger", "Fractal Blossom" };
    // for (const std::string& name : escortNames)
    // {
    //     Pandora::EntitySharedPtr pEscort = SpawnShip(name, "/models/flagship/light_carrier/light_carrier.glb");
    //     pEscort->AddComponent<DiceComponent>();
    //     m_pPlayerFleet->AddShip(pEscort);
    // }
}

Pandora::EntitySharedPtr Sector::SpawnShip(const std::string& name, const std::string& modelPath)
{
    using namespace Pandora;

    Pandora::EntitySharedPtr pShip = CreateEntity();

    TransformComponent& transformComponent = pShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // pShip->AddComponent<ModelComponent>(modelPath);
    pShip->AddComponent<ShipNavigationComponent>();
    pShip->AddComponent<NameComponent>(name);

    return pShip;
}

Fleet* Sector::GetPlayerFleet() const
{
    return m_pPlayerFleet.get();
}

} // namespace WingsOfSteel::TheBrightestStar