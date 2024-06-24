#pragma once

namespace WingsOfSteel::Pandora
{

class ImGuiSystem;
class RenderSystem;
class VFS;
class Window;

void Initialize();
void Update();
void Shutdown();

ImGuiSystem* GetImGuiSystem();
RenderSystem* GetRenderSystem();
VFS* GetVFS();
Window* GetWindow();

} // namespace WingsOfSteel::Pandora