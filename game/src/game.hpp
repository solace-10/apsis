#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

#include "orbit_camera.hpp"
#include "test_entity.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class Game
{
public:
    Game();
    ~Game();

    void Initialize();
    void Update(float delta);
    void Shutdown();

private:
    OrbitCameraSharedPtr m_pCamera;
    Pandora::SceneSharedPtr m_pScene;

    TestEntitySharedPtr m_pTestEntity;
};

} // namespace WingsOfSteel::TheBrightestStar