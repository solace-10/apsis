#pragma once

namespace WingsOfSteel::Pandora
{

class RenderSystem;

void Initialize();
void Shutdown();

RenderSystem* GetRenderSystem();

} // namespace WingsOfSteel::Pandora