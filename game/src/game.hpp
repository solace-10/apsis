#pragma once

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(PrefabEditor);
DECLARE_SMART_PTR(Sector);

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

    UI::PrefabEditor* GetPrefabEditor();
    Sector* GetSector();

    static Game* Get();

private:
    void DrawImGuiMenuBar();

    SceneSharedPtr m_pMenuScene;
    SectorSharedPtr m_pSector;

    UI::PrefabEditorUniquePtr m_pPrefabEditor;
};

inline UI::PrefabEditor* Game::GetPrefabEditor()
{
    return m_pPrefabEditor.get();
}

inline Sector* Game::GetSector()
{
    return m_pSector.get();
}

} // namespace WingsOfSteel