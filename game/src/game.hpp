#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(ItemManager);
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

    const ItemManager* GetItemManager() const;

private:
    void DrawImGuiMenuBar();

    Pandora::SceneSharedPtr m_pMenuScene;

    ItemManagerUniquePtr m_pItemManager;
    SectorInfoSharedPtr m_pSectorInfo;
    SubSectorSharedPtr m_pSubSector;
    SectorGeneratorUniquePtr m_pSectorGenerator;
};

inline const ItemManager* Game::GetItemManager() const
{
    return m_pItemManager.get();
}

} // namespace WingsOfSteel::TheBrightestStar