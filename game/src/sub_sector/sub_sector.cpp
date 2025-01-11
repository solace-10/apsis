#include <imgui.h>

#include <render/debug_render.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <pandora.hpp>

#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/sub_sector_camera_component.hpp"
#include "sector/sector_info.hpp"
#include "sub_sector/sub_sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SubSector::SubSector()
{

}

SubSector::~SubSector()
{

}

void SubSector::Initialize()
{
    using namespace Pandora;

    Scene::Initialize();

    AddSystem(std::make_unique<PlayerControllerSystem>());
    AddSystem(std::make_unique<ShipNavigationSystem>());

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem(std::make_unique<CameraSystem>());
    AddSystem(std::make_unique<DebugRenderSystem>());

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->AddComponent<SubSectorCameraComponent>();
    subSectorCameraComponent.SetOffset(12.0f, 9.0f, -55.0f);
    subSectorCameraComponent.SetTarget(0.0f, 0.0f, 1000.0f);
    subSectorCameraComponent.SetMaximumDrift(0.25f, 0.15f, 0.0f);
    SetCamera(m_pCamera); 

    SpawnDome();
    SpawnPlayerShip();
    subSectorCameraComponent.SetAnchorEntity(m_pPlayerShip);
}

void SubSector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    Pandora::GetDebugRender()->Line(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1000.0f),
        Pandora::Color::White
    );

    Pandora::GetDebugRender()->Line(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1000.0f, 0.0f),
        Pandora::Color::Red
    );

    DrawCameraDebugUI();
}

void SubSector::ShowCameraDebugUI(bool state)
{
    m_ShowCameraDebugUI = state;
}

void SubSector::DrawCameraDebugUI()
{
    if (!m_ShowCameraDebugUI)
    {
        return;
    }

    ImGui::Begin("Camera", &m_ShowCameraDebugUI);

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->GetComponent<SubSectorCameraComponent>();

    const glm::vec3& offset = subSectorCameraComponent.GetOffset();
    float foffset[3] = { offset.x, offset.y, offset.z };
    if (ImGui::InputFloat3("Offset", foffset))
    {
        subSectorCameraComponent.SetOffset(foffset[0], foffset[1], foffset[2]);
    }

    const glm::vec3& drift = subSectorCameraComponent.GetMaximumDrift();
    float fdrift[3] = { drift.x, drift.y, drift.z };
    if (ImGui::InputFloat3("Drift", fdrift))
    {
        subSectorCameraComponent.SetMaximumDrift(fdrift[0], fdrift[1], fdrift[2]);
    }

    ImGui::End();
}

void SubSector::SpawnDome()
{
    using namespace Pandora;

    m_pDome = CreateEntity();

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->GetComponent<SubSectorCameraComponent>();

    TransformComponent& transformComponent = m_pDome->AddComponent<TransformComponent>();
    transformComponent.transform = glm::scale(glm::translate(glm::mat4(1.0f), subSectorCameraComponent.GetOffset() + glm::vec3(0.0f, 0.0f, 100.0f)), glm::vec3(20.0f));

    m_pDome->AddComponent<ModelComponent>("/models/dome/dome.glb");
}

void SubSector::SpawnPlayerShip()
{
    using namespace Pandora;

    m_pPlayerShip = CreateEntity();

    // DebugRenderComponent& debugRenderComponent = m_pPlayerShip->AddComponent<DebugRenderComponent>();
    // debugRenderComponent.color = Color::Cyan;
    // debugRenderComponent.shape = DebugRenderShape::Box;
    // debugRenderComponent.length = 50.0f;
    // debugRenderComponent.width = 25.0f;
    // debugRenderComponent.height = 10.0f;

    TransformComponent& transformComponent = m_pPlayerShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    m_pPlayerShip->AddComponent<ModelComponent>("/models/flagship/light_carrier/light_carrier.glb");
    m_pPlayerShip->AddComponent<ShipNavigationComponent>();
    m_pPlayerShip->AddComponent<PlayerControllerComponent>();
}

} // namespace WingsOfSteel::TheBrightestStar