#include <cassert>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

Entity::Entity()
: m_pScene(nullptr)
, m_Transform(1.0f)
, m_MarkedForRemoval(false)
{

}
    
Entity::~Entity()
{

}

void Entity::OnAddedToScene(Scene* pScene)
{
    assert(!m_pScene);
    m_pScene = pScene;
}

void Entity::OnRemovedFromScene()
{
    assert(m_pScene);
    m_pScene = nullptr;
}

void Entity::Update(float delta)
{

}

void Entity::Render()
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