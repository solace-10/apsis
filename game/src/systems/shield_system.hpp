#pragma once

#include <scene/components/transform_component.hpp>
#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

namespace WingsOfSteel
{

class ShieldSystem : public System
{
public:
    ShieldSystem() = default;
    ~ShieldSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;
};

} // namespace WingsOfSteel
