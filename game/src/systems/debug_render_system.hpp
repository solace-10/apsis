#pragma once

#include <scene/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class DebugRenderSystem : public Pandora::System
{
public:
    DebugRenderSystem() {};
    ~DebugRenderSystem() {};

    void Initialize() override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar