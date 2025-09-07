#pragma once

#include <glm/vec3.hpp>

#include <scene/components/rigid_body_component.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class NavigationSystem : public System
{
public:
    NavigationSystem() = default;
    virtual ~NavigationSystem() = default;

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override {}

    void TurnTowards(const glm::vec3& position, RigidBodyComponent& rigidBodyComponent, float torque);
};

} // namespace WingsOfSteel