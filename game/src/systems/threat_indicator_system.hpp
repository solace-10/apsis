#pragma once

#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

class ThreatIndicatorSystem : public System
{
public:
    ThreatIndicatorSystem() = default;
    ~ThreatIndicatorSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;
};

} // namespace WingsOfSteel
