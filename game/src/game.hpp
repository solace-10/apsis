#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(SectorGenerator);
DECLARE_SMART_PTR(SectorInfo);
DECLARE_SMART_PTR(SubSector);

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

    SectorInfoSharedPtr m_pSectorInfo;
    SubSectorSharedPtr m_pSubSector;
    SectorGeneratorUniquePtr m_pSectorGenerator;
};

} // namespace WingsOfSteel::TheBrightestStar