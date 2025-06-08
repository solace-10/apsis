#pragma once

#include <memory>

#include "scene/systems/system.hpp"

#include "physics/physics_visualization.hpp"

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(PhysicsSimulationSystem);
class PhysicsSimulationSystem : public System
{
public:
    PhysicsSimulationSystem();
    ~PhysicsSimulationSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void OnRigidBodyCreated(entt::registry& registry, entt::entity entity);
    void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity);

    PhysicsVisualization* GetVizualisation() { return m_pPhysicsVisualization.get(); }

private:
    Scene* m_pScene{ nullptr };
    std::unique_ptr<btDefaultCollisionConfiguration> m_pCollisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_pDispatcher;
    std::unique_ptr<btDbvtBroadphase> m_pBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_pWorld;
    PhysicsVisualizationUniquePtr m_pPhysicsVisualization;
};

} // namespace WingsOfSteel::Pandora