#include <cassert>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

Entity::Entity(Scene* pScene)
: m_pScene(pScene)
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

} // namespace WingsOfSteel::Pandora