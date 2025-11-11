#pragma once

#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>
#include <scene/systems/system.hpp>

#include "components/shield_component.hpp"

namespace WingsOfSteel
{

class ShieldSystem : public System
{
public:
    ShieldSystem() = default;
    ~ShieldSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

private:
    void UpdateShieldState(float delta, ShieldComponent& shieldComponent, ModelComponent& modelComponent);
};

} // namespace WingsOfSteel
