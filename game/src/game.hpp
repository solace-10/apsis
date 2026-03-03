#pragma once

#include <memory>

#include "core/smart_ptr.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Sector);
class WebInterop;

class Game
{
public:
    Game();
    ~Game();

    void Initialize();
    void Update(float delta);
    void Shutdown();

    Sector* GetSector();

    static Game* Get();

private:
    void DrawImGuiMenuBar();

    SectorSharedPtr m_pSector;

#if defined(TARGET_PLATFORM_WEB)
    std::unique_ptr<WebInterop> m_pWebInterop;
#endif
};

inline Sector* Game::GetSector()
{
    return m_pSector.get();
}

} // namespace WingsOfSteel
