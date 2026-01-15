#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class SpaceObjectRenderSystem : public System
{
public:
    SpaceObjectRenderSystem();
    ~SpaceObjectRenderSystem();

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;
};

} // namespace WingsOfSteel
