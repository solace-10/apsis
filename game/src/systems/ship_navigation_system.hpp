#pragma once

#include "systems/navigation_system.hpp"

namespace WingsOfSteel
{

class ShipNavigationSystem : public NavigationSystem
{
public:
    ShipNavigationSystem() {};
    ~ShipNavigationSystem() {};

    void Initialize(Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel