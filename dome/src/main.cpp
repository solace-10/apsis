#include "pandora.hpp"
#include "dome.hpp"

int main() 
{
    using namespace WingsOfSteel;

    Pandora::WindowSettings windowSettings;
    windowSettings.SetSize(2080, 832);
    windowSettings.SetTitle("Dome");

    Dome::Dome dome;
    Pandora::Initialize(
        windowSettings,
        [&dome](){ dome.Initialize(); },
        [&dome](float delta){ dome.Update(delta); },
        [&dome](){ dome.Shutdown(); }
    );
}