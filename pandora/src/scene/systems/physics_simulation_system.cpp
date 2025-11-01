#include "scene/systems/physics_simulation_system.hpp"

#include "pandora.hpp"
#include "physics/physics_visualization.hpp"
#include "scene/components/ghost_component.hpp"
#include "scene/components/rigid_body_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/scene.hpp"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <algorithm>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace WingsOfSteel
{

PhysicsSimulationSystem::PhysicsSimulationSystem()
{
    m_pCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    m_pDispatcher = std::make_unique<btCollisionDispatcher>(m_pCollisionConfiguration.get());
    m_pBroadphase = std::make_unique<btDbvtBroadphase>();
    m_pSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_pWorld = std::make_unique<btDiscreteDynamicsWorld>(m_pDispatcher.get(), m_pBroadphase.get(), m_pSolver.get(), m_pCollisionConfiguration.get());
    m_pWorld->setGravity(btVector3(0, 0, 0));

    m_pPhysicsVisualization = std::make_unique<PhysicsVisualization>(m_pWorld.get());
}

PhysicsSimulationSystem::~PhysicsSimulationSystem()
{
    m_pScene->GetRegistry().on_construct<RigidBodyComponent>().disconnect<&PhysicsSimulationSystem::OnRigidBodyCreated>(this);
    m_pScene->GetRegistry().on_destroy<RigidBodyComponent>().disconnect<&PhysicsSimulationSystem::OnRigidBodyDestroyed>(this);
    m_pScene->GetRegistry().on_construct<GhostComponent>().disconnect<&PhysicsSimulationSystem::OnGhostComponentCreated>(this);
    m_pScene->GetRegistry().on_destroy<GhostComponent>().disconnect<&PhysicsSimulationSystem::OnGhostComponentDestroyed>(this);
}

void PhysicsSimulationSystem::Initialize(Scene* pScene)
{
    m_pScene = pScene;
    m_pScene->GetRegistry().on_construct<RigidBodyComponent>().connect<&PhysicsSimulationSystem::OnRigidBodyCreated>(this);
    m_pScene->GetRegistry().on_destroy<RigidBodyComponent>().connect<&PhysicsSimulationSystem::OnRigidBodyDestroyed>(this);
    m_pScene->GetRegistry().on_construct<GhostComponent>().connect<&PhysicsSimulationSystem::OnGhostComponentCreated>(this);
    m_pScene->GetRegistry().on_destroy<GhostComponent>().connect<&PhysicsSimulationSystem::OnGhostComponentDestroyed>(this);
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

    if (!m_GhostEntitiesToAdd.empty())
    {
        for (auto& entityToAdd : m_GhostEntitiesToAdd)
        {
            GhostComponent& ghostComponent = m_pScene->GetRegistry().get<GhostComponent>(entityToAdd.entity);
            if (ghostComponent.GetBulletGhostObject())
            {
                m_pWorld->addCollisionObject(ghostComponent.GetBulletGhostObject());
                entityToAdd.added = true;
            }
        }

        m_GhostEntitiesToAdd.erase(std::remove_if(m_GhostEntitiesToAdd.begin(), m_GhostEntitiesToAdd.end(),
                                       [](const EntityToAdd& entityToAdd) { return entityToAdd.added; }),
            m_GhostEntitiesToAdd.end());
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

void PhysicsSimulationSystem::OnGhostComponentCreated(entt::registry& registry, entt::entity entity)
{
    GhostComponent& ghostComponent = m_pScene->GetRegistry().get<GhostComponent>(entity);
    if (ghostComponent.GetBulletGhostObject())
    {
        m_pWorld->addCollisionObject(ghostComponent.GetBulletGhostObject());
    }
    else
    {
        m_GhostEntitiesToAdd.emplace_back(entity);
    }
}

void PhysicsSimulationSystem::OnGhostComponentDestroyed(entt::registry& registry, entt::entity entity)
{
    GhostComponent& ghostComponent = registry.get<GhostComponent>(entity);
    if (ghostComponent.GetBulletGhostObject())
    {
        m_pWorld->removeCollisionObject(ghostComponent.GetBulletGhostObject());
    }
}

std::optional<PhysicsSimulationSystem::RaycastResult> PhysicsSimulationSystem::Raycast(const glm::vec3& from, const glm::vec3& to)
{
    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(btFrom, btTo);
    m_pWorld->rayTest(btFrom, btTo, rayCallback);

    if (rayCallback.hasHit())
    {
        RaycastResult result;
        result.position = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());

        // Find the entity associated with the hit collision object
        const btCollisionObject* pCollisionObject = rayCallback.m_collisionObject;

        // Try to cast to rigid body first
        const btRigidBody* pRigidBody = btRigidBody::upcast(pCollisionObject);
        if (pRigidBody)
        {
            result.pEntity = RigidBodyComponent::GetEntityFromRigidBody(pRigidBody);
            return result;
        }

        // If not a rigid body, try ghost object
        const btGhostObject* pGhostObject = btGhostObject::upcast(pCollisionObject);
        if (pGhostObject)
        {
            result.pEntity = GhostComponent::GetEntityFromGhostObject(pGhostObject);
            return result;
        }
    }

    return std::nullopt;
}

void PhysicsSimulationSystem::SetCollisionBetween(EntitySharedPtr pEntity1, EntitySharedPtr pEntity2, bool enable)
{
    if (!pEntity1 || !pEntity2)
    {
        return;
    }

    if (!pEntity1->HasComponent<RigidBodyComponent>() || !pEntity2->HasComponent<RigidBodyComponent>())
    {
        return;
    }

    RigidBodyComponent& rb1 = pEntity1->GetComponent<RigidBodyComponent>();
    RigidBodyComponent& rb2 = pEntity2->GetComponent<RigidBodyComponent>();

    btRigidBody* pBulletRB1 = rb1.GetBulletRigidBody();
    btRigidBody* pBulletRB2 = rb2.GetBulletRigidBody();

    if (pBulletRB1 && pBulletRB2)
    {
        pBulletRB1->setIgnoreCollisionCheck(pBulletRB2, !enable);
        pBulletRB2->setIgnoreCollisionCheck(pBulletRB1, !enable);
    }
}

} // namespace WingsOfSteel
