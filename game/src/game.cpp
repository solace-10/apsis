#include <pandora.hpp>

#include "scene/camera.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"
#include "pandora.hpp"

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
    m_pCamera = std::make_shared<Pandora::Camera>();

    m_pScene = std::make_shared<Pandora::Scene>();
    m_pScene->AddEntity(m_pCamera);
    m_pScene->SetCamera(m_pCamera);
    Pandora::SetActiveScene(m_pScene);

    m_pTestEntity = std::make_shared<TestEntity>();
    m_pScene->AddEntity(m_pTestEntity);
}

void Game::Update(float delta)
{

}

void Game::Shutdown()
{

}

} // namespace WingsOfSteel::TheBrightestStar