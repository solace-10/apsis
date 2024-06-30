#include "pandora.hpp"
#include "game.hpp"

int main() 
{
    using namespace WingsOfSteel;

    TheBrightestStar::Game game;
    Pandora::Initialize(
        [&game](){ game.Initialize(); },
        [&game](float delta){ game.Update(delta); },
        [&game](){ game.Shutdown(); }
    );

}