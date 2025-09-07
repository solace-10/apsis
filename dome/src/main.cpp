#include "pandora.hpp"
#include "dome.hpp"

int main() 
{
    using namespace WingsOfSteel;

    WindowSettings windowSettings;
    windowSettings.SetSize(2080, 832);
    windowSettings.SetTitle("Dome");

    Dome dome;
    Initialize(
        windowSettings,
        [&dome](){ dome.Initialize(); },
        [&dome](float delta){ dome.Update(delta); },
        [&dome](){ dome.Shutdown(); }
    );
}