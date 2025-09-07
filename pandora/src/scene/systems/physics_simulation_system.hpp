#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>

#include "scene/systems/system.hpp"

#include "physics/physics_visualization.hpp"

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);

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

    PhysicsVisualization* GetVisualization() { return m_pPhysicsVisualization.get(); }

    struct RaycastResult
    {
        EntitySharedPtr pEntity;
        glm::vec3 position;
    };

    std::optional<RaycastResult> Raycast(const glm::vec3& from, const glm::vec3& to);

private:
    Scene* m_pScene{ nullptr };
    std::unique_ptr<btDefaultCollisionConfiguration> m_pCollisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_pDispatcher;
    std::unique_ptr<btDbvtBroadphase> m_pBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_pWorld;
    PhysicsVisualizationUniquePtr m_pPhysicsVisualization;

    struct EntityToAdd
    {
        EntityToAdd(entt::entity _entity) : entity(_entity), added(false) {}

        entt::entity entity;
        bool added;
    };

    std::vector<EntityToAdd> m_EntitiesToAdd;
};

} // namespace WingsOfSteel