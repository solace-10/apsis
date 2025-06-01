#include <btBulletCollisionCommon.h>

#include "physics/internal/bullet_debug_render.hpp"
#include "physics/physics_visualization.hpp"

namespace WingsOfSteel::Pandora
{

PhysicsVisualization::PhysicsVisualization(btCollisionWorld* pWorld)
    : m_pWorld(pWorld)
{
    m_pDebugRender = std::make_unique<BulletDebugRender>();
    m_pWorld->setDebugDrawer(m_pDebugRender.get());
}

PhysicsVisualization::~PhysicsVisualization()
{
    m_pWorld->setDebugDrawer(nullptr);
}

void PhysicsVisualization::Update()
{
    m_pWorld->debugDrawWorld();
}

void PhysicsVisualization::SetEnabled(Mode mode, bool state)
{
    m_pDebugRender->SetEnabled(mode, state);
}

bool PhysicsVisualization::IsEnabled(Mode mode) const
{
    return m_pDebugRender->IsEnabled(mode);
}

} // namespace WingsOfSteel::Pandora