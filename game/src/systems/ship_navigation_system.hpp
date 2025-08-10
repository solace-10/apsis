#pragma once

#include "systems/navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class ShipNavigationSystem : public NavigationSystem
{
public:
    ShipNavigationSystem() {};
    ~ShipNavigationSystem() {};

    void Initialize(Pandora::Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar