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
    m_pCamera->LookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    m_pScene = std::make_shared<Pandora::Scene>();
    m_pScene->AddEntity(m_pCamera);
    m_pScene->SetCamera(m_pCamera);
    Pandora::SetActiveScene(m_pScene);

    m_pTestEntity = std::make_shared<TestEntity>();
    m_pScene->AddEntity(m_pTestEntity);
}

void Game::Update(float delta)
{
    static float accum = 0.0f;
    accum += delta;

    m_pCamera->LookAt(
        glm::vec3(cosf(accum) * 10.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

void Game::Shutdown()
{

}

} // namespace WingsOfSteel::TheBrightestStar