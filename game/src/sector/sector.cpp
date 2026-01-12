#include <imgui.h>

#include <core/log.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/ambient_light_component.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/directional_light_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/planet_component.hpp"
#include "components/sector_camera_component.hpp"
#include "sector/sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/planet_render_system.hpp"

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
    AddSystem<PlanetRenderSystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);
    OrbitCameraComponent& orbitCameraComponent = m_pCamera->AddComponent<OrbitCameraComponent>();
    orbitCameraComponent.distance = 50.0f;
    orbitCameraComponent.maximumDistance = 100.0f;
    SetCamera(m_pCamera);

    SpawnLight();

    m_pEarth = CreateEntity();
    m_pEarth->AddComponent<PlanetComponent>();
}

void Sector::Update(float delta)
{
    Scene::Update(delta);

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

void Sector::SpawnLight()
{
    m_pLight = CreateEntity();

    DirectionalLightComponent& directionalLightComponent = m_pLight->AddComponent<DirectionalLightComponent>();
    directionalLightComponent.SetAngle(226.0f);
    directionalLightComponent.SetPitch(40.0f);
    directionalLightComponent.SetColor(1.0f, 0.96f, 0.68f);

    AmbientLightComponent& ambientLightComponent = m_pLight->AddComponent<AmbientLightComponent>();
    ambientLightComponent.SetColor(0.10f, 0.14f, 0.17f);
}

} // namespace WingsOfSteel
