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

void Game::EntryPoint()
{
    Pandora::Initialize();

    m_pCamera = std::make_shared<Pandora::Camera>();

    Pandora::SceneSharedPtr pScene = std::make_shared<Pandora::Scene>();
    pScene->AddEntity(m_pCamera);
    pScene->SetCamera(m_pCamera);
    Pandora::SetActiveScene(pScene);
}

} // namespace WingsOfSteel::TheBrightestStar