#include "game.hpp"
#include "pandora.hpp"

int main()
{
    using namespace WingsOfSteel;

    WindowSettings windowSettings;
    windowSettings.SetSize(1440, 900);
    windowSettings.SetTitle("Crimson Sun II");

    Game game;
    Initialize(
        windowSettings,
        [&game]() { game.Initialize(); },
        [&game](float delta) { game.Update(delta); },
        [&game]() { game.Shutdown(); });
}