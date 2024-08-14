#include <pandora.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>

#include "sector/sector.hpp"
#include "sector/sector_generator.hpp"
#include "sector/sector_info.hpp"
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
    m_pSectorGenerator = std::make_unique<SectorGenerator>();

    m_pCamera = std::make_shared<OrbitCamera>();
    m_pCamera->LookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    m_pMenuScene = std::make_shared<Pandora::Scene>();
    m_pMenuScene->AddEntity(m_pCamera);
    m_pMenuScene->SetCamera(m_pCamera);
    Pandora::SetActiveScene(m_pMenuScene);

    //m_pTestEntity = std::make_shared<TestEntity>();
    //m_pScene->AddEntity(m_pTestEntity);
}

void Game::Update(float delta)
{
    if (m_pSector == nullptr && m_pSectorGenerator->CanCreate(0))
    {
        SectorInfoSharedPtr pSectorInfo = m_pSectorGenerator->Create(0);
        m_pSector = std::make_shared<Sector>(pSectorInfo);
        m_pSector->Initialize();
        Pandora::SetActiveScene(m_pSector);
    }
}

void Game::Shutdown()
{

}

} // namespace WingsOfSteel::TheBrightestStar