#include <glm/glm.hpp>
#include <imgui.h>

#include <core/log.hpp>
#include <core/serialization.hpp>
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
#include "components/metadata_component.hpp"
#include "components/orbital_elements_component.hpp"
#include "components/planet_component.hpp"
#include "components/sector_camera_component.hpp"
#include "game.hpp"
#include "sector/database.hpp"
#include "sector/group_filters.hpp"
#include "sector/sector.hpp"
#include "space/earth_frame.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/orbit_propagation_system.hpp"
#include "systems/planet_render_system.hpp"
#include "systems/space_object_picking_system.hpp"
#include "systems/space_object_render_system.hpp"
#include "systems/sun_system.hpp"

#if defined(TARGET_PLATFORM_WEB)
#include "emscripten/web_interop.hpp"
#endif

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
    AddSystem<OrbitPropagationSystem>();
    AddSystem<SunSystem>();
    AddSystem<SpaceObjectPickingSystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    // SpaceObjectRenderSystem projects the space objects into screen space, so it has to run after
    // CameraSystem has moved the camera for this frame.
    AddSystem<SpaceObjectRenderSystem>();

    m_pCamera = CreateEntity();
    // Near/far planes for orbital viewing (kilometers)
    m_pCamera->AddComponent<CameraComponent>(70.0f, 10.0f, 200000.0f);
    OrbitCameraComponent& orbitCameraComponent = m_pCamera->AddComponent<OrbitCameraComponent>();
    orbitCameraComponent.distance = 30000.0f;
    orbitCameraComponent.wantedDistance = 20000.0f;
    orbitCameraComponent.minimumDistance = 8000.0f;
    orbitCameraComponent.maximumDistance = 100000.0f;
    orbitCameraComponent.zoomSensitivity = 1000.0f;
    orbitCameraComponent.sensitivity = 6.0f;
    orbitCameraComponent.minimumPitch = glm::radians(-80.0f);
    orbitCameraComponent.maximumPitch = glm::radians(80.0f);
    SetCamera(m_pCamera);

    SpawnLight();

    constexpr float kEarthSemiMajorRadius = static_cast<float>(kEarthSemiMajorAxis); // Equatorial radius
    constexpr float kEarthSemiMinorRadius = static_cast<float>(kEarthSemiMinorAxis); // Polar radius

    m_pEarth = CreateEntity();
    m_pEarth->AddComponent<TransformComponent>();

    PlanetComponent& planetComponent = m_pEarth->AddComponent<PlanetComponent>();
    planetComponent.semiMajorRadius = kEarthSemiMajorRadius;
    planetComponent.semiMinorRadius = kEarthSemiMinorRadius;

    // Atmospheric scattering using Sean O'Neil's algorithm
    // The atmosphere height is computed automatically as 2.5% of planet radius (~159km for Earth)
    // to match O'Neil's scale function calibration
    AtmosphereComponent& atmosphereComponent = m_pEarth->AddComponent<AtmosphereComponent>();
    atmosphereComponent.Kr = 0.0015f; // Rayleigh scattering constant (reduced for thinner atmosphere)
    atmosphereComponent.Km = 0.0005f; // Mie scattering constant
    atmosphereComponent.ESun = 2.0f; // Sun brightness
    atmosphereComponent.g = -0.950f; // Mie phase asymmetry
    atmosphereComponent.wavelength = glm::vec3(0.650f, 0.570f, 0.475f); // RGB wavelengths (micrometers)
    atmosphereComponent.scaleDepth = 0.25f; // Scale height
    atmosphereComponent.numSamples = 16; // Ray march samples

    InitializeGroupFilters();
    InitializeDatabase();
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

#if defined(TARGET_PLATFORM_WEB)
    if (EntitySharedPtr pSelected = m_pSelectedSpaceObject.lock())
    {
        if (WebInterop* pWebInterop = WebInterop::GetInstance())
        {
            pWebInterop->NotifySpaceObjectUpdated(pSelected);
        }
    }
#endif
}

void Sector::InitializeDatabase()
{
    m_pDatabase = std::make_unique<Database>();
    m_pDatabase->GetAllObjects(
        [](const Json::Data& data) {
            Sector* pSector = Game::Get()->GetSector();
            if (!pSector)
            {
                return;
            }

            if (!data.contains("objects") || !data["objects"].is_array())
            {
                Log::Error() << "Server response missing 'objects' array.";
                return;
            }

            if (!data.contains("groups") || !data["groups"].is_object())
            {
                Log::Error() << "Server response missing 'groups' object.";
                return;
            }

            pSector->InitializeSpaceObjects(data["objects"], data["groups"]);
        },
        [](const std::string& error) {});
}

void Sector::InitializeGroupFilters()
{
    m_pGroupFilters = std::make_unique<GroupFilters>();
    m_pGroupFilters->RegisterGroupFilter("last-30-days", "Last 30 days' launches", "#CC6600", true);
    m_pGroupFilters->RegisterGroupFilter("stations", "Space Stations", "#FFD700", true);
    m_pGroupFilters->RegisterGroupFilter("starlink", "Starlink", "#4A90D9", false);
    m_pGroupFilters->RegisterGroupFilter("oneweb", "OneWeb", "#7B68EE", true);
    m_pGroupFilters->RegisterGroupFilter("gps-ops", "GPS", "#32CD32", true);
    m_pGroupFilters->RegisterGroupFilter("gnss", "GNSS", "#FF6347", false);
    m_pGroupFilters->RegisterGroupFilter("geo", "Active geosynchronous", "#00CED1", false);
    m_pGroupFilters->RegisterGroupFilter("science", "Science", "#87CEEB", true);
    m_pGroupFilters->RegisterGroupFilter("fengyun-1c-debris", "Chinese ASAT test debris", "#880040", false);
    m_pGroupFilters->RegisterGroupFilter("debris", "Debris", "#808080", false);
    m_pGroupFilters->RegisterGroupFilter("analyst", "Well-tracked analyst", "#404040", false);
    m_pGroupFilters->RegisterGroupFilter("other", "Other", "#DD8080", false);
}

void Sector::InitializeSpaceObjects(const Json::Data& objectsData, const Json::Data& groupsData)
{
    // Identify the highest NORAD Id so we can get pre-allocate a vector.
    // Right now the Ids are always below <100k, but this will almost certainly change in the future.
    // Alternatively we could start at 100k and then just double as necessary.
    int32_t highestNoradId = 0;
    for (const auto& objectData : objectsData)
    {
        auto result = Json::TryDeserializeInteger(nullptr, objectData, "norad_id");
        if (!result.has_value() || result.value() <= 0)
        {
            Log::Warning() << "Object with missing or invalid 'norad_id', skipping.";
            continue;
        }
        highestNoradId = glm::max(result.value(), highestNoradId);
    }

    m_NoradIdIndex.resize(highestNoradId + 1);

    Log::Info() << "Highest NORAD Id: " << highestNoradId;
    for (const auto& objectData : objectsData)
    {
        EntitySharedPtr pEntity = CreateEntity();

        OrbitalElementsComponent& orbitalElementsComponent = pEntity->AddComponent<OrbitalElementsComponent>();
        orbitalElementsComponent.Deserialize(nullptr, objectData);

        MetadataComponent& metadataComponent = pEntity->AddComponent<MetadataComponent>();
        metadataComponent.Deserialize(nullptr, objectData);
        
        pEntity->AddComponent<TransformComponent>();

        m_NoradIdIndex[orbitalElementsComponent.GetNoradId()] = pEntity;
    }

    Log::Info() << "Loaded " << objectsData.size() << " space objects.";

    for (const std::string& groupFilterName : m_pGroupFilters->GetGroupFilterNames())
    {
        // The "other" group is dynamically generated at runtime and won't be part of the received data.
        if (groupFilterName == "other")
        {
            continue;
        }
        
        if (!groupsData.contains(groupFilterName))
        {
            Log::Warning() << "Group '" << groupFilterName << "': no data found in server response.";
            continue;
        }

        const auto& idsData = groupsData[groupFilterName];
        if (!idsData.is_array())
        {
            Log::Warning() << "Group '" << groupFilterName << "': expected array of NORAD IDs, got different type.";
            continue;
        }
        GroupFilter* pGroupFilter = m_pGroupFilters->GetGroupFilter(groupFilterName);
        size_t idsInGroup = 0;
        for (const auto& idData : idsData)
        {
            if (!idData.is_number_integer())
            {
                Log::Warning() << "Group '" << groupFilterName << "': expected integer NORAD ID, got non-integer type.";
                continue;
            }

            const int32_t idFromData = idData.get<int32_t>();
            if (idFromData <= 0)
            {
                Log::Warning() << "Group '" << groupFilterName << "': invalid NORAD ID " << idFromData << ", expected positive integer.";
                continue;
            }

            const size_t id = static_cast<size_t>(idFromData);
            if (id >= m_NoradIdIndex.size())
            {
                Log::Warning() << "Group '" << groupFilterName << "': NORAD ID " << id << " exceeds highest known ID (" << (m_NoradIdIndex.size() - 1) << ").";
                continue;
            }

            EntitySharedPtr pEntity = m_NoradIdIndex[id];
            if (!pEntity)
            {
                Log::Warning() << "Group '" << groupFilterName << "': NORAD ID " << id << " not found in loaded object data.";
                continue;
            }
            
            MetadataComponent& metadataComponent = pEntity->GetComponent<MetadataComponent>();
            metadataComponent.AddToGroupFilter(pGroupFilter);
            idsInGroup++;
        }

        pGroupFilter->SetCount(idsInGroup);
        Log::Info() << "Group '" << pGroupFilter->GetName() << "': " << pGroupFilter->GetCount() << " objects.";
    }

    InitializeOtherGroupFilter();

#if defined(TARGET_PLATFORM_WEB)
    if (WebInterop* pWebInterop = WebInterop::GetInstance())
    {
        pWebInterop->NotifyGroupFiltersChanged(m_pGroupFilters.get());
    }
#endif

    SpaceObjectRenderSystem* pSpaceObjectSystem = GetSystem<SpaceObjectRenderSystem>();
    if (pSpaceObjectSystem)
    {
        pSpaceObjectSystem->NotifyGroupFiltersChanged();
    }

    const uint32_t hubbleNoradId = 20580;
    if (m_NoradIdIndex[hubbleNoradId])
    {
        SetSelectedSpaceObject(m_NoradIdIndex[hubbleNoradId]);
    }
}

void Sector::InitializeOtherGroupFilter()
{
    GroupFilter* pOtherGroupFilter = m_pGroupFilters->GetGroupFilter("other");
    if (!pOtherGroupFilter)
    {
        Log::Error() << "Missing 'other' group filter.";
        return;
    }

    uint32_t count = 0;
    for (EntitySharedPtr pEntity : m_NoradIdIndex)
    {
        if (!pEntity)
        {
            continue;
        }

        MetadataComponent& metadataComponent = pEntity->GetComponent<MetadataComponent>();
        if (metadataComponent.GetGroupFilterMask() == 0)
        {
            metadataComponent.AddToGroupFilter(pOtherGroupFilter);
            count++;
        }
    }

    pOtherGroupFilter->SetCount(count);
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

    // No direction is set here. SunSystem overwrites it every frame with where the Sun actually
    // is, which is the whole point of the light; anything set here would only be seen on the
    // first frame.
    DirectionalLightComponent& directionalLightComponent = m_pLight->AddComponent<DirectionalLightComponent>();
    directionalLightComponent.SetColor(1.0f, 0.96f, 0.90f); // Approximation for the sun (type G star at a temperature of 5778K)

    AmbientLightComponent& ambientLightComponent = m_pLight->AddComponent<AmbientLightComponent>();
    ambientLightComponent.SetColor(0.0f, 0.0f, 0.0f);
}

void Sector::SetSelectedSpaceObject(EntitySharedPtr pEntity)
{
    m_pSelectedSpaceObject = pEntity;

#if defined(TARGET_PLATFORM_WEB)
    if (WebInterop* pWebInterop = WebInterop::GetInstance())
    {
        if (pEntity)
        {
            pWebInterop->NotifySpaceObjectSelected(pEntity);
        }
        else
        {
            pWebInterop->NotifySpaceObjectDeselected();
        }
    }
#endif
}

EntitySharedPtr Sector::GetEntityByNoradId(uint32_t noradId) const
{
    if (noradId >= m_NoradIdIndex.size())
    {
        return nullptr;
    }
    else
    {
        return m_NoradIdIndex[noradId];
    }
}

} // namespace WingsOfSteel
