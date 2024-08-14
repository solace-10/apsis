#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

#include "orbit_camera.hpp"
#include "test_entity.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(Sector);
DECLARE_SMART_PTR(SectorGenerator);

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
    Pandora::SceneSharedPtr m_pMenuScene;

    //TestEntitySharedPtr m_pTestEntity;
    SectorSharedPtr m_pSector;
    SectorGeneratorUniquePtr m_pSectorGenerator;
};

} // namespace WingsOfSteel::TheBrightestStar