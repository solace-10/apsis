#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

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
    void DrawImGuiMenuBar();

    Pandora::SceneSharedPtr m_pMenuScene;

    SectorSharedPtr m_pSector;
    SectorGeneratorUniquePtr m_pSectorGenerator;
};

} // namespace WingsOfSteel::TheBrightestStar