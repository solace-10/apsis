#pragma once

#include <memory>

#include "scene/systems/system.hpp"

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace WingsOfSteel::Pandora
{

class PhysicsSimulationSystem : public System
{
public:
    PhysicsSimulationSystem();
    ~PhysicsSimulationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void OnRigidBodyCreated(entt::registry& registry, entt::entity entity);
    void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity);

private:
    Scene* m_pScene{nullptr};
    std::unique_ptr<btDefaultCollisionConfiguration> m_pCollisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_pDispatcher;
    std::unique_ptr<btDbvtBroadphase> m_pBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_pWorld;
};

} // namespace WingsOfSteel::Pandora