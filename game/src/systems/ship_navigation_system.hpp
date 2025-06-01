#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class ShipNavigationSystem : public Pandora::System
{
public:
    ShipNavigationSystem() {};
    ~ShipNavigationSystem() {};

    void Initialize(Pandora::Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar