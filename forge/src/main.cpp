#include "pandora.hpp"
#include "forge.hpp"

int main() 
{
    using namespace WingsOfSteel;

    Forge::Forge forge;
    Pandora::Initialize(
        [&forge](){ forge.Initialize(); },
        [&forge](float delta){ forge.Update(delta); },
        [&forge](){ forge.Shutdown(); }
    );
}