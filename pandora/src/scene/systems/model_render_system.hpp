#pragma once

#include <webgpu/webgpu_cpp.h>

#include "scene/systems/system.hpp"

namespace WingsOfSteel::Pandora
{

class ModelRenderSystem : public System
{
public:
    ModelRenderSystem();
    ~ModelRenderSystem();

    void Initialize() override {};
    void Update(float delta) override {};

    void Render(wgpu::RenderPassEncoder& renderPass);

private:

};

} // namespace WingsOfSteel::Pandora