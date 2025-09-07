#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class DebugRenderSystem : public System
{
public:
    DebugRenderSystem() {};
    ~DebugRenderSystem() {};

    void Initialize(Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel