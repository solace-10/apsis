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

#include "components/ai_strikecraft_controller_component.hpp"
#include "components/faction_component.hpp"
#include "components/player_controller_component.hpp"
#include "components/sector_camera_component.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "entity_builder/entity_builder.hpp"
#include "systems/ai_strategic_system.hpp"
#include "systems/ai_strikecraft_controller_system.hpp"
#include "systems/ammo_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/mech_navigation_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"
#include "systems/target_overlay_system.hpp"
#include "systems/weapon_system.hpp"

namespace WingsOfSteel
{

Sector::Sector()
{
}

Sector::~Sector()
{
}

void Sector::Initialize()
{
    Scene::Initialize();

    AddSystem<ModelRenderSystem>();
    AddSystem<PhysicsSimulationSystem>();
    AddSystem<AIStrategicSystem>();
    AddSystem<AIStrikecraftControllerSystem>();
    AddSystem<PlayerControllerSystem>();
    AddSystem<MechNavigationSystem>();
    AddSystem<ShipNavigationSystem>();
    AddSystem<WeaponSystem>();
    AddSystem<AmmoSystem>();
    AddSystem<TargetOverlaySystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->AddComponent<SectorCameraComponent>();
    sectorCameraComponent.position = glm::vec3(0.0f, 45.0f, 30.0f);
    sectorCameraComponent.target = glm::vec3(0.0f, 0.0f, 0.0f);
    sectorCameraComponent.maximumDrift = glm::vec3(0.0f, 0.0f, 0.0f);
    SetCamera(m_pCamera);

    SpawnDome();
    SpawnPlayerFleet();

    m_pEncounter = std::make_unique<Encounter>();
    m_pEncounter->Initialize(std::static_pointer_cast<Sector>(shared_from_this()));
}

void Sector::Update(float delta)
{
    Scene::Update(delta);

    m_pEncounter->Update(delta);

    if (m_ShowGrid)
    {
        GetDebugRender()->XZSquareGrid(-1000.0f, 1000.0f, -1.0f, 100.0f, Color::White);
    }
    
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
    m_pDome = CreateEntity();

    SectorCameraComponent& sectorCameraComponent = m_pCamera->GetComponent<SectorCameraComponent>();

    TransformComponent& transformComponent = m_pDome->AddComponent<TransformComponent>();
    transformComponent.transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    ModelComponent& modelComponent = m_pDome->AddComponent<ModelComponent>();
    modelComponent.SetModel("/models/dome/dome.glb");
}

void Sector::SpawnPlayerFleet()
{
    SceneWeakPtr pWeakScene = weak_from_this();
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/player/mech.json", glm::translate(glm::mat4(1.0f), glm::vec3(-200.0f, 0.0f, 0.0f)), [pWeakScene](EntitySharedPtr pEntity){
        SectorSharedPtr pScene = std::dynamic_pointer_cast<Sector>(pWeakScene.lock());
        if (pScene)
        {
            pScene->m_pPlayerMech = pEntity;
            pEntity->AddComponent<PlayerControllerComponent>();
            pScene->m_pCamera->GetComponent<SectorCameraComponent>().anchorEntity = pEntity;

            pScene->GetSystem<WeaponSystem>()->AttachWeapon(
                "/entity_prefabs/weapons/mech/autocannon_r.json",
                pEntity,
                "RightArm"
            );

            pScene->GetSystem<WeaponSystem>()->AttachWeapon(
                "/entity_prefabs/weapons/mech/rotary_cannon_l.json",
                pEntity,
                "LeftArm"
            );
        }
    });

    EntityBuilder::Build(pWeakScene, "/entity_prefabs/player/carrier.json", glm::translate(glm::mat4(1.0f), glm::vec3(-250.0f, 0.0f, 0.0f)), [pWeakScene](EntitySharedPtr pEntity){
        SectorSharedPtr pScene = std::dynamic_pointer_cast<Sector>(pWeakScene.lock());
        if (pScene)
        {
            pScene->m_pCarrier = pEntity;
        }
    });

    // std::array<std::string, 2> escortNames = { "Skyforger", "Fractal Blossom" };
    // for (const std::string& name : escortNames)
    // {
    //     EntitySharedPtr pEscort = SpawnShip(name, "/models/flagship/light_carrier/light_carrier.glb");
    //     pEscort->AddComponent<DiceComponent>();
    //     m_pPlayerFleet->AddShip(pEscort);
    // }
}

} // namespace WingsOfSteel