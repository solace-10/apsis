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

#include "components/player_controller_component.hpp"
#include "components/sector_camera_component.hpp"
#include "fleet.hpp"
#include "sector/sector.hpp"
#include "ship_builder/ship_builder.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"
#include "systems/weapon_system.hpp"

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
    AddSystem<WeaponSystem>();

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
    SpawnEnemyFleet();
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

    const glm::vec2 mousePos = ImGui::GetIO().MousePos;
    const glm::vec3 worldPos = GetSystem<CameraSystem>()->MouseToWorld(mousePos);
    Pandora::GetDebugRender()->Circle(worldPos, glm::vec3(0.0f, 1.0f, 0.0f), Pandora::Color::BlueViolet, 1.0f, 16.0f);
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
    m_pPlayerFleet = std::make_shared<Fleet>();

    m_pPlayerShip = CreateEntity();
    ShipBuilder::Build(m_pPlayerShip, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
    m_pPlayerShip->AddComponent<PlayerControllerComponent>();
    m_pPlayerFleet->AddShip(m_pPlayerShip);

    // std::array<std::string, 2> escortNames = { "Skyforger", "Fractal Blossom" };
    // for (const std::string& name : escortNames)
    // {
    //     Pandora::EntitySharedPtr pEscort = SpawnShip(name, "/models/flagship/light_carrier/light_carrier.glb");
    //     pEscort->AddComponent<DiceComponent>();
    //     m_pPlayerFleet->AddShip(pEscort);
    // }
}

void Sector::SpawnEnemyFleet()
{
    m_pEnemyFleet = std::make_shared<Fleet>();

    Pandora::EntitySharedPtr pShip = CreateEntity();
    ShipBuilder::Build(pShip, glm::translate(glm::mat4(1.0f), glm::vec3(-120.0f, 0.0f, 0.0f)));
    m_pEnemyFleet->AddShip(pShip);

    pShip = CreateEntity();
    ShipBuilder::Build(pShip, glm::translate(glm::mat4(1.0f), glm::vec3(120.0f, 0.0f, 50.0f)));
    m_pEnemyFleet->AddShip(pShip);
}

Fleet* Sector::GetPlayerFleet() const
{
    return m_pPlayerFleet.get();
}

} // namespace WingsOfSteel::TheBrightestStar