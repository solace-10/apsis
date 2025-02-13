#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::TheBrightestStar
{

DECLARE_SMART_PTR(PrefabEditor);
DECLARE_SMART_PTR(Hyperscape);
DECLARE_SMART_PTR(ItemManager);
DECLARE_SMART_PTR(Sector);
DECLARE_SMART_PTR(SubSector);

namespace UI
{
    DECLARE_SMART_PTR(PrefabEditor);
}

class Game
{
public:
    Game();
    ~Game();

    void Initialize();
    void Update(float delta);
    void Shutdown();

    const ItemManager* GetItemManager() const;
    UI::PrefabEditor* GetPrefabEditor();

    static Game* Get();

private:
    void DrawImGuiMenuBar();

    Pandora::SceneSharedPtr m_pMenuScene;

    ItemManagerUniquePtr m_pItemManager;
    HyperscapeUniquePtr m_pHyperscape;
    SectorSharedPtr m_pSector;
    SubSectorSharedPtr m_pSubSector;

    UI::PrefabEditorUniquePtr m_pPrefabEditor;
};

inline const ItemManager* Game::GetItemManager() const
{
    return m_pItemManager.get();
}

inline UI::PrefabEditor* Game::GetPrefabEditor()
{
    return m_pPrefabEditor.get();
}

} // namespace WingsOfSteel::TheBrightestStar