#pragma once

#include "core/smart_ptr.hpp"
#include "scene/camera.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class Game
{
public:
    Game();
    ~Game();

    //void Update(float delta);

    void EntryPoint();

private:
    Pandora::CameraSharedPtr m_pCamera;
    Pandora::SceneSharedPtr m_pScene;
};

} // namespace WingsOfSteel::TheBrightestStar