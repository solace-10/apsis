#include "pandora.hpp"
#include "forge.hpp"

int main() 
{
    using namespace WingsOfSteel;

    Pandora::WindowSettings windowSettings;
    windowSettings.SetSize(900, 600);
    windowSettings.SetTitle("Forge");

    Forge::Forge forge;
    Pandora::Initialize(
        windowSettings,
        [&forge](){ forge.Initialize(); },
        [&forge](float delta){ forge.Update(delta); },
        [&forge](){ forge.Shutdown(); }
    );
}