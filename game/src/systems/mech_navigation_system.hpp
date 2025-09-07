#pragma once

#include <scene/systems/system.hpp>
#include "navigation_system.hpp"

namespace WingsOfSteel
{

class MechNavigationSystem : public NavigationSystem
{
public:
    MechNavigationSystem() {};
    ~MechNavigationSystem() {};

    void Initialize(Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel