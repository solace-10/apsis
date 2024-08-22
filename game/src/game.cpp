#include <imgui/imgui_system.hpp>
#include <pandora.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

#include "sector/sector_generator.hpp"
#include "sector/sector_info.hpp"
#include "sector/sub_sector.hpp"
#include "sector/sub_sector_info.hpp"
#include "game.hpp"

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

    m_pSectorGenerator = std::make_unique<SectorGenerator>();

    // m_pCamera = std::make_shared<OrbitCamera>();
    // m_pCamera->LookAt(
    //     glm::vec3(0.0f, 0.0f, 10.0f),
    //     glm::vec3(0.0f, 0.0f, 0.0f),
    //     glm::vec3(0.0f, 1.0f, 0.0f)
    // );

    m_pMenuScene = std::make_shared<Pandora::Scene>();
    // m_pMenuScene->AddEntity(m_pCamera);
    // m_pMenuScene->SetCamera(m_pCamera);
    Pandora::SetActiveScene(m_pMenuScene);

    //m_pTestEntity = std::make_shared<TestEntity>();
    //m_pScene->AddEntity(m_pTestEntity);
}

void Game::Update(float delta)
{
    if (m_pSubSector == nullptr && m_pSectorGenerator->CanCreate(0))
    {
        m_pSectorInfo = m_pSectorGenerator->Create(0);
        SubSectorInfoSharedPtr pSubSectorInfo = m_pSectorInfo->CreateSubSectorInfo();
        m_pSubSector = std::make_shared<SubSector>(pSubSectorInfo);
        m_pSubSector->Initialize();
        Pandora::SetActiveScene(m_pSubSector);
    }
}

void Game::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Game::DrawImGuiMenuBar()
{
    if (m_pSubSector)
    {
        if (ImGui::BeginMenu("Sector"))
        {
            static bool sShowSignalsWindow = false;
            if (ImGui::MenuItem("Signals", nullptr, &sShowSignalsWindow))
            {
                m_pSubSector->ShowSignalsDebugUI(sShowSignalsWindow);
            }
            ImGui::EndMenu();
        }
    }
}

} // namespace WingsOfSteel::TheBrightestStar