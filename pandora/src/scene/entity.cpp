#include <cassert>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

Entity::Entity(Scene* pScene)
: m_pScene(pScene)
, m_Transform(1.0f)
, m_MarkedForRemoval(false)
{

}
    
Entity::~Entity()
{

}

void Entity::OnAddedToScene()
{
}

void Entity::OnRemovedFromScene()
{
    assert(m_pScene);
    m_pScene = nullptr;
}

void Entity::Render(wgpu::RenderPassEncoder renderPass)
{

}

void Entity::SetTransform(const glm::mat4x4& transform)
{
    m_Transform = transform;
}

const glm::mat4x4& Entity::GetTransform() const
{
    return m_Transform;
}

} // namespace WingsOfSteel::Pandora