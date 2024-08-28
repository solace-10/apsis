#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "forge.hpp"

namespace WingsOfSteel::Forge
{

Forge::Forge()
{

}

Forge::~Forge()
{

}

void Forge::Initialize()
{
    Pandora::GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar();});
}

void Forge::Update(float delta)
{

}

void Forge::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Forge::DrawImGuiMenuBar()
{

}

} // namespace WingsOfSteel::TheBrightestStar