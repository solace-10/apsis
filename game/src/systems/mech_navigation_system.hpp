#pragma once

#include <scene/systems/system.hpp>
#include "navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

class MechNavigationSystem : public NavigationSystem
{
public:
    MechNavigationSystem() {};
    ~MechNavigationSystem() {};

    void Initialize(Pandora::Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar