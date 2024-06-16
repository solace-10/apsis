#pragma once

namespace WingsOfSteel::Pandora
{

class RenderSystem;
class Window;

void Initialize();
void Update();
void Shutdown();

RenderSystem* GetRenderSystem();
Window* GetWindow();

} // namespace WingsOfSteel::Pandora