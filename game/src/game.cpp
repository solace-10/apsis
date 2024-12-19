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
#include "sub_sector/signal/sub_sector_signal.hpp"
#include "sub_sector/sub_sector.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Game::Game()
{

}

Game::~Game()
{

}

void Game::Initialize()
{
    Pandora::GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar();});

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
}

} // namespace WingsOfSteel::TheBrightestStar