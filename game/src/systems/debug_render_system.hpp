#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class DebugRenderSystem : public Pandora::System
{
public:
    DebugRenderSystem() {};
    ~DebugRenderSystem() {};

    void Initialize(Pandora::Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar