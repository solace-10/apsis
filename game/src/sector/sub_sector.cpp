#include <imgui.h>

#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>

#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "sector/sector_info.hpp"
#include "sector/sub_sector.hpp"
#include "sector/sub_sector_info.hpp"
#include "sector/signal/signal.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SubSector::SubSector(SubSectorInfoSharedPtr& pSubSectorInfo)
: m_pSubSectorInfo(pSubSectorInfo)
, m_ShowSignalsDebugUI(false)
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

    OrbitCameraComponent& orbitCameraComponent = m_pCamera->AddComponent<OrbitCameraComponent>();
    orbitCameraComponent.distance = 50.0f;
    orbitCameraComponent.orbitAngle = glm::radians(-90.0f);
    orbitCameraComponent.pitch = 0.0f;
    orbitCameraComponent.minimumPitch = glm::radians(0.0f);
    orbitCameraComponent.maximumPitch = glm::radians(80.0f);
    SetCamera(m_pCamera); 

    for (auto& pSignal : m_pSubSectorInfo->GetSignals())
    {
        pSignal->Spawn(this);
    }

    SpawnPlayerShip();
    orbitCameraComponent.anchorEntity = m_pPlayerShip;

    DrawSignalsDebugUI();
}

void SubSector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    DrawSignalsDebugUI();
}

void SubSector::ShowSignalsDebugUI(bool state)
{
    m_ShowSignalsDebugUI = state;
}

void SubSector::DrawSignalsDebugUI()
{
    if (!m_ShowSignalsDebugUI)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(512, 512));
    ImGui::Begin("Signals", &m_ShowSignalsDebugUI);

    if (ImGui::BeginTable("table", 4, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Difficulty");
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Z");
        ImGui::TableHeadersRow();

        SectorInfoSharedPtr pSectorInfo = m_pSubSectorInfo->GetSectorInfo().lock();
        if (pSectorInfo)
        {
            for (auto& pSignal : pSectorInfo->GetSignals())
            {
                ImGui::TableNextRow();
                const glm::vec3& pos = pSignal->GetPosition();
                ImGui::TableNextColumn(); ImGui::Text("%s", pSignal->GetName().c_str());
                ImGui::TableNextColumn(); ImGui::Text("%.2f", pSignal->GetSignalDifficulty());
                ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.x));
                ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.z));
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void SubSector::SpawnPlayerShip()
{
    using namespace Pandora;

    m_pPlayerShip = CreateEntity();

    DebugRenderComponent& debugRenderComponent = m_pPlayerShip->AddComponent<DebugRenderComponent>();
    debugRenderComponent.color = Color::Cyan;
    debugRenderComponent.shape = DebugRenderShape::Box;
    debugRenderComponent.length = 50.0f;
    debugRenderComponent.width = 25.0f;
    debugRenderComponent.height = 10.0f;

    TransformComponent& transformComponent = m_pPlayerShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    m_pPlayerShip->AddComponent<ModelComponent>("/models/flagship/light_carrier/light_carrier.glb");
    m_pPlayerShip->AddComponent<ShipNavigationComponent>();
    m_pPlayerShip->AddComponent<PlayerControllerComponent>();
}

} // namespace WingsOfSteel::TheBrightestStar