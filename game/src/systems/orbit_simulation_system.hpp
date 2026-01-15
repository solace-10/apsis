
#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class OrbitSimulationSystem : public System
{
public:
    OrbitSimulationSystem();
    ~OrbitSimulationSystem();

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

private:
};

} // namespace WingsOfSteel
