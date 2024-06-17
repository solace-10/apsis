#pragma once

namespace WingsOfSteel::Pandora
{

class RenderSystem;
class VFS;
class Window;

void Initialize();
void Update();
void Shutdown();

RenderSystem* GetRenderSystem();
VFS* GetVFS();
Window* GetWindow();

} // namespace WingsOfSteel::Pandora