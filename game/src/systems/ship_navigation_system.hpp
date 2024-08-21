#pragma once

#include <scene/system.hpp>

namespace WingsOfSteel::TheBrightestStar
{

class ShipNavigationSystem : public Pandora::System
{
public:
    ShipNavigationSystem() {};
    ~ShipNavigationSystem() {};

    void Initialize() override {};
    void Update(float delta) override;
};

} // namespace WingsOfSteel::TheBrightestStar