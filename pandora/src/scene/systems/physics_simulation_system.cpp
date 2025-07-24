#include "scene/systems/physics_simulation_system.hpp"

#include "pandora.hpp"
#include "physics/physics_visualization.hpp"
#include "scene/components/rigid_body_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace WingsOfSteel::Pandora
{

PhysicsSimulationSystem::PhysicsSimulationSystem()
{
    m_pCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    m_pDispatcher = std::make_unique<btCollisionDispatcher>(m_pCollisionConfiguration.get());
    m_pBroadphase = std::make_unique<btDbvtBroadphase>();
    m_pSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_pWorld = std::make_unique<btDiscreteDynamicsWorld>(m_pDispatcher.get(), m_pBroadphase.get(), m_pSolver.get(), m_pCollisionConfiguration.get());
    // m_pWorld->setInternalTickCallback( &InternalTickCallback, &m_CollisionDataSet );
    m_pWorld->setGravity(btVector3(0, 0, 0));

    m_pPhysicsVisualization = std::make_unique<PhysicsVisualization>(m_pWorld.get());
}

PhysicsSimulationSystem::~PhysicsSimulationSystem()
{
    m_pScene->GetRegistry().on_construct<RigidBodyComponent>().disconnect<&PhysicsSimulationSystem::OnRigidBodyCreated>(this);
    m_pScene->GetRegistry().on_destroy<RigidBodyComponent>().disconnect<&PhysicsSimulationSystem::OnRigidBodyDestroyed>(this);
}

void PhysicsSimulationSystem::Initialize(Scene* pScene)
{
    m_pScene = pScene;
    m_pScene->GetRegistry().on_construct<RigidBodyComponent>().connect<&PhysicsSimulationSystem::OnRigidBodyCreated>(this);
    m_pScene->GetRegistry().on_destroy<RigidBodyComponent>().connect<&PhysicsSimulationSystem::OnRigidBodyDestroyed>(this);
}

void PhysicsSimulationSystem::Update(float delta)
{
    if (!m_EntitiesToAdd.empty())
    {
        for (auto& entityToAdd : m_EntitiesToAdd)
        {
            RigidBodyComponent& rigidBodyComponent = m_pScene->GetRegistry().get<RigidBodyComponent>(entityToAdd.entity);
            if (rigidBodyComponent.GetBulletRigidBody())
            {
                m_pWorld->addRigidBody(rigidBodyComponent.GetBulletRigidBody());
                entityToAdd.added = true;
            }
        }

        m_EntitiesToAdd.erase(std::remove_if(m_EntitiesToAdd.begin(), m_EntitiesToAdd.end(), 
            [](const EntityToAdd& entityToAdd) { return entityToAdd.added; }), 
            m_EntitiesToAdd.end());
    }

    m_pWorld->stepSimulation(delta, 5);
    m_pPhysicsVisualization->Update();

    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const RigidBodyComponent, TransformComponent>();

    view.each([](const auto entity, const RigidBodyComponent& rigidBodyComponent, TransformComponent& transformComponent) {
        transformComponent.transform = rigidBodyComponent.GetWorldTransform();
    });
}

void PhysicsSimulationSystem::OnRigidBodyCreated(entt::registry& registry, entt::entity entity)
{
    RigidBodyComponent& rigidBodyComponent = m_pScene->GetRegistry().get<RigidBodyComponent>(entity);
    if (rigidBodyComponent.GetBulletRigidBody())
    {
        m_pWorld->addRigidBody(rigidBodyComponent.GetBulletRigidBody());
    }
    else
    {
        m_EntitiesToAdd.emplace_back(entity);
    }
}

void PhysicsSimulationSystem::OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity)
{
    RigidBodyComponent& rigidBodyComponent = registry.get<RigidBodyComponent>(entity);
    if (rigidBodyComponent.GetBulletRigidBody())
    {
        m_pWorld->removeRigidBody(rigidBodyComponent.GetBulletRigidBody());
    }
}

} // namespace WingsOfSteel::Pandora