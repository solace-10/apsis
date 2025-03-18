#include <imgui/imgui_system.hpp>
#include <pandora.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

#include "hyperscape/hyperscape.hpp"
#include "items/item_info.hpp"
#include "items/item_manager.hpp"
#include "game.hpp"
#include "sector/signal/sector_signal.hpp"
#include "sector/sector.hpp"
#include "sub_sector/encounters/encounter_editor.hpp"
#include "sub_sector/signal/sub_sector_signal.hpp"
#include "sub_sector/sub_sector.hpp"
#include "ui/prefab_editor.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Game* g_pGame = nullptr;

Game::Game()
{

}

Game::~Game()
{

}

Game* Game::Get()
{
    return g_pGame;
}

void Game::Initialize()
{
    g_pGame = this;

    Pandora::GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar();});

    m_pPrefabEditor = std::make_unique<UI::PrefabEditor>();
    m_pPrefabEditor->Initialize();

    m_pEncounterEditor = std::make_unique<EncounterEditor>();

    m_pItemManager = std::make_unique<ItemManager>();
    //m_pSectorGenerator = std::make_unique<SectorGenerator>();

    m_pHyperscape = std::make_unique<Hyperscape>();
    m_pHyperscape->Initialize();

    auto sectorSignals = m_pHyperscape->GetSignals();
    assert(sectorSignals.size() == 1);
    SectorSignal* pSectorSignal = sectorSignals.back();

    m_pSector = pSectorSignal->Spawn();

    auto subSectorSignals = m_pSector->GetSignals();
    assert(subSectorSignals.size() == 1);
    SubSectorSignal* pSubSectorSignal = subSectorSignals.back();

    m_pSubSector = pSubSectorSignal->Spawn();
    Pandora::SetActiveScene(m_pSubSector);

    // m_pCamera = std::make_shared<OrbitCamera>();
    // m_pCamera->LookAt(
    //     glm::vec3(0.0f, 0.0f, 10.0f),
    //     glm::vec3(0.0f, 0.0f, 0.0f),
    //     glm::vec3(0.0f, 1.0f, 0.0f)
    // );

    //m_pMenuScene = std::make_shared<Pandora::Scene>();
    // m_pMenuScene->AddEntity(m_pCamera);
    // m_pMenuScene->SetCamera(m_pCamera);
    //Pandora::SetActiveScene(m_pMenuScene);
}

void Game::Update(float delta)
{
    m_pPrefabEditor->DrawPrefabEditor();
    m_pEncounterEditor->Update();
}

void Game::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Game::DrawImGuiMenuBar()
{
    if (m_pSubSector)
    {
        if (ImGui::BeginMenu("Sub-sector"))
        {
            static bool sShowCameraWindow = false;
            if (ImGui::MenuItem("Camera", nullptr, &sShowCameraWindow))
            {
                m_pSubSector->ShowCameraDebugUI(sShowCameraWindow);
            }
            ImGui::EndMenu();
        }
    }

    if (ImGui::BeginMenu("UI"))
    {
        static bool sShowPrefabEditor = false;
        if (ImGui::MenuItem("Prefab Editor", nullptr, &sShowPrefabEditor))
        {
            m_pPrefabEditor->ShowPrefabEditor(sShowPrefabEditor);
        }
        ImGui::EndMenu();
    }
}

} // namespace WingsOfSteel::TheBrightestStar