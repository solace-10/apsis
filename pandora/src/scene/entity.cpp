#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

Entity::Entity()
: m_Transform(1.0f)
{

}
    
Entity::~Entity()
{

}

void Entity::Initialize()
{

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