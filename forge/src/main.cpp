#include "pandora.hpp"
#include "forge.hpp"

int main() 
{
    using namespace WingsOfSteel;

    WindowSettings windowSettings;
    windowSettings.SetSize(900, 600);
    windowSettings.SetTitle("Forge");

    Forge forge;
    Initialize(
        windowSettings,
        [&forge](){ forge.Initialize(); },
        [&forge](float delta){ forge.Update(delta); },
        [&forge](){ forge.Shutdown(); }
    );
}