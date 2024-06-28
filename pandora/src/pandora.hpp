#pragma once

namespace WingsOfSteel::Pandora
{

class ImGuiSystem;
class RenderSystem;
class ResourceSystem;
class VFS;
class Window;

void Initialize();
void Update();
void Shutdown();

ImGuiSystem* GetImGuiSystem();
RenderSystem* GetRenderSystem();
ResourceSystem* GetResourceSystem();
VFS* GetVFS();
Window* GetWindow();

} // namespace WingsOfSteel::Pandora