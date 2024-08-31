#include "pandora.hpp"
#include "game.hpp"

int main() 
{
    using namespace WingsOfSteel;

    Pandora::WindowSettings windowSettings;
    windowSettings.SetSize(1440, 900);
    windowSettings.SetTitle("The Brightest Star");

    TheBrightestStar::Game game;
    Pandora::Initialize(
        windowSettings,
        [&game](){ game.Initialize(); },
        [&game](float delta){ game.Update(delta); },
        [&game](){ game.Shutdown(); }
    );
}