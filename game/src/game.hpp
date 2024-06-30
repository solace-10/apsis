#pragma once

#include "core/smart_ptr.hpp"
#include "scene/camera.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

#include "testentity.hpp"

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
    Pandora::CameraSharedPtr m_pCamera;
    Pandora::SceneSharedPtr m_pScene;

    TestEntitySharedPtr m_pTestEntity;
};

} // namespace WingsOfSteel::TheBrightestStar