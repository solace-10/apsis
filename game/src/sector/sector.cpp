#include <imgui.h>

#include <core/log.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/ambient_light_component.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/directional_light_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/atmosphere_component.hpp"
#include "components/planet_component.hpp"
#include "components/sector_camera_component.hpp"
#include "components/space_object_component.hpp"
#include "sector/sector.hpp"
#include "resources/resource.fwd.hpp"
#include "space_objects/space_object.hpp"
#include "space_objects/space_object_catalogue.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/orbit_simulation_system.hpp"
#include "systems/planet_render_system.hpp"
#include "systems/space_object_render_system.hpp"

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
    AddSystem<OrbitSimulationSystem>();
    AddSystem<SpaceObjectRenderSystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    m_pCamera = CreateEntity();
    // Near/far planes for orbital viewing (kilometers)
    m_pCamera->AddComponent<CameraComponent>(70.0f, 10.0f, 200000.0f);
    OrbitCameraComponent& orbitCameraComponent = m_pCamera->AddComponent<OrbitCameraComponent>();
    orbitCameraComponent.distance = 20000.0f; // ~3x Earth radius for full planet view
    orbitCameraComponent.minimumDistance = 7000.0f; // Just above surface
    orbitCameraComponent.maximumDistance = 100000.0f;
    SetCamera(m_pCamera);

    SpawnLight();

    // Earth's WGS84 ellipsoid dimensions in kilometers
    constexpr float kEarthSemiMajorRadius = 6378.137f; // Equatorial radius
    constexpr float kEarthSemiMinorRadius = 6356.752f; // Polar radius

    m_pEarth = CreateEntity();
    PlanetComponent& planetComponent = m_pEarth->AddComponent<PlanetComponent>();
    planetComponent.semiMajorRadius = kEarthSemiMajorRadius;
    planetComponent.semiMinorRadius = kEarthSemiMinorRadius;

    // Atmospheric scattering using Sean O'Neil's algorithm
    // The atmosphere height is computed automatically as 2.5% of planet radius (~159km for Earth)
    // to match O'Neil's scale function calibration
    AtmosphereComponent& atmosphereComponent = m_pEarth->AddComponent<AtmosphereComponent>();
    atmosphereComponent.Kr = 0.0015f; // Rayleigh scattering constant (reduced for thinner atmosphere)
    atmosphereComponent.Km = 0.0005f; // Mie scattering constant
    atmosphereComponent.ESun = 15.0f; // Sun brightness
    atmosphereComponent.g = -0.950f; // Mie phase asymmetry
    atmosphereComponent.wavelength = glm::vec3(0.650f, 0.570f, 0.475f); // RGB wavelengths (micrometers)
    atmosphereComponent.scaleDepth = 0.25f; // Scale height
    atmosphereComponent.numSamples = 5; // Ray march samples

    InitializeSpaceObjectCatalogue();
}

void Sector::Update(float delta)
{
    Scene::Update(delta);

    if (m_ShowGrid)
    {
        // Grid scaled for planetary viewing (kilometers): 200,000 km extent, 10,000 km spacing
        GetDebugRender()->XZSquareGrid(-100000.0f, 100000.0f, -1.0f, 10000.0f, Color::White);
    }

    DrawCameraDebugUI();
}

void Sector::InitializeSpaceObjectCatalogue()
{
    m_pSpaceObjectCatalogue = std::make_unique<SpaceObjectCatalogue>();
    
    GetResourceSystem()->RequestResource("/celestrak/stations.json", [this](ResourceSharedPtr pResource) {
        ResourceDataStoreSharedPtr pResourceDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        SpaceObjectCatalogue* pCatalogue = GetSpaceObjectCatalogue();
        size_t successfulEntries = 0;
        for (const Json::Data& data : pResourceDataStore->Data())
        {
            SpaceObject spaceObject;
            if (spaceObject.DeserializeOMM(data))
            {
                pCatalogue->Add(spaceObject);
                successfulEntries++;

                EntitySharedPtr pEntity = CreateEntity();
                SpaceObjectComponent& spaceObjectComponent = pEntity->AddComponent<SpaceObjectComponent>();
                spaceObjectComponent.AssignSpaceObject(spaceObject);
                pEntity->AddComponent<TransformComponent>();
            }
            else
            {
                Log::Warning() << "Failed to deserialize OMM from " << pResourceDataStore->GetPath();
            }
        }

        Log::Info() << "Added " << successfulEntries << " to space object catalogue.";
    });
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
    directionalLightComponent.SetColor(1.0f, 0.96f, 0.90f); // Approximation for the sun (type G star at a temperature of 5778K)

    AmbientLightComponent& ambientLightComponent = m_pLight->AddComponent<AmbientLightComponent>();
    ambientLightComponent.SetColor(0.0f, 0.0f, 0.0f);
}

} // namespace WingsOfSteel
