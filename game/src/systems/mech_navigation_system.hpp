#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class MechNavigationSystem : public Pandora::System
{
public:
    MechNavigationSystem() {};
    ~MechNavigationSystem() {};

    void Initialize(Pandora::Scene* pScene) override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar