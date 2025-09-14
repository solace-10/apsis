#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class TargetOverlaySystem : public System
{
public:
    TargetOverlaySystem() {};
    ~TargetOverlaySystem() {};

    void Initialize(Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel