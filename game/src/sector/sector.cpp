#include <imgui.h>

#include <render/debug_render.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <pandora.hpp>

#include "components/dice_component.hpp"
#include "components/name_component.hpp"
#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/sector_camera_component.hpp"
#include "dice/dice_generator.hpp"
#include "sector/encounters/encounter_window.hpp"
#include "sector/encounters/encounter.hpp"
#include "sector/sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"
#include "fleet.hpp"

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

    AddSystem(std::make_unique<PlayerControllerSystem>());
    AddSystem(std::make_unique<ShipNavigationSystem>());

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem(std::make_unique<CameraSystem>());
    AddSystem(std::make_unique<DebugRenderSystem>());

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->AddComponent<SectorCameraComponent>();
    sectorCameraComponent.position = glm::vec3(0.0f, 200.0f, -75.0f);
    sectorCameraComponent.target = glm::vec3(0.0f, 0.0f, 0.0f);
    sectorCameraComponent.maximumDrift = glm::vec3(0.0f, 0.0f, 0.0f);
    SetCamera(m_pCamera); 

    m_pEncounterWindow = std::make_shared<EncounterWindow>();
    m_pEncounterWindow->Initialize("/ui/prefabs/encounter.json");

    SpawnDome();
    SpawnPlayerFleet();
    SpawnEncounter();
    sectorCameraComponent.anchorEntity = m_pPlayerShip;
}

void Sector::Update(float delta)
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

    Pandora::GetDebugRender()->XZSquareGrid(-1000.0f, 1000.0f, 0.0f, 100.0f, Pandora::Color::White);

    if (m_pEncounter)
    {
        m_pEncounter->Update(delta);
    }

    if (m_pEncounterWindow)
    {
        m_pEncounterWindow->Render();
    }

    DrawCameraDebugUI();
}

void Sector::ShowCameraDebugUI(bool state)
{
    m_ShowCameraDebugUI = state;
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

void Sector::SpawnEncounter()
{
    using namespace Pandora;

    std::string encounterName = "h0_asteroid_field_1";
    const std::string path = "/encounters/" + encounterName + ".json";
    GetResourceSystem()->RequestResource(path, [this, encounterName](ResourceSharedPtr pResource)
    {
        ResourceDataStoreSharedPtr pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        if (pDataStore)
        {
            GenerateDice();
            m_pEncounter = std::make_shared<Encounter>(encounterName, pDataStore);
            m_pEncounter->Bind(m_pEncounterWindow);
            m_pEncounter->Start();
        }
    });
}

void Sector::ForceEncounter(EncounterSharedPtr pEncounter)
{
    if (m_pEncounter)
    {
        m_pEncounter->Stop();
    }

    GenerateDice();
    m_pEncounter = pEncounter;
    m_pEncounter->Bind(m_pEncounterWindow);
    m_pEncounter->Start();
}

void Sector::SpawnDome()
{
    using namespace Pandora;

    m_pDome = CreateEntity();

    SectorCameraComponent& sectorCameraComponent = m_pCamera->GetComponent<SectorCameraComponent>();

    TransformComponent& transformComponent = m_pDome->AddComponent<TransformComponent>();
    transformComponent.transform = glm::scale(glm::translate(glm::mat4(1.0f), sectorCameraComponent.position + glm::vec3(0.0f, 0.0f, 100.0f)), glm::vec3(20.0f));

    m_pDome->AddComponent<ModelComponent>("/models/dome/dome.glb");
}

void Sector::SpawnPlayerFleet()
{
    using namespace Pandora;

    m_pPlayerFleet = std::make_shared<Fleet>();

    m_pPlayerShip = SpawnShip("Everflame", "/models/flagship/light_carrier/light_carrier.glb");
    m_pPlayerShip->AddComponent<PlayerControllerComponent>();
    m_pPlayerShip->AddComponent<DiceComponent>();
    m_pPlayerFleet->AddShip(m_pPlayerShip);

    std::array<std::string, 2> escortNames = { "Skyforger", "Fractal Blossom" };
    for (const std::string& name : escortNames)
    {
        Pandora::EntitySharedPtr pEscort = SpawnShip(name, "/models/flagship/light_carrier/light_carrier.glb");
        pEscort->AddComponent<DiceComponent>();
        m_pPlayerFleet->AddShip(pEscort);
    }
}

Pandora::EntitySharedPtr Sector::SpawnShip(const std::string& name, const std::string& modelPath)
{
    using namespace Pandora;

    Pandora::EntitySharedPtr pShip = CreateEntity();
    
    TransformComponent& transformComponent = pShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    pShip->AddComponent<ModelComponent>(modelPath);
    pShip->AddComponent<ShipNavigationComponent>();
    pShip->AddComponent<NameComponent>(name);

    return pShip;
}

void Sector::GenerateDice()
{
    using namespace Pandora;
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<DiceComponent>();

    view.each([](const auto entity, DiceComponent& diceComponent)
    {
        for (size_t categoryIdx = 0; categoryIdx < magic_enum::enum_count<DiceCategory>(); categoryIdx++)
        {
            DiceCategory category = static_cast<DiceCategory>(categoryIdx);
            DiceComponent::DiceContainer& diceContainer = diceComponent.GetDice(category);
            std::vector<Dice> rolledDice = DiceGenerator::Generate(diceContainer.size());
            assert(rolledDice.size() == diceContainer.size());
            for (size_t i = 0; i < diceContainer.size(); i++)
            {
                diceContainer[i] = rolledDice[i];
            }
        }
    });
}

Fleet* Sector::GetPlayerFleet() const
{
    return m_pPlayerFleet.get();
}

} // namespace WingsOfSteel::TheBrightestStar