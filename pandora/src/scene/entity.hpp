#pragma once

#include <glm/mat4x4.hpp>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

class Entity
{
public:
    Entity();
    virtual ~Entity();

    virtual void Initialize();
    virtual void Update(float delta);
    virtual void Render();

    void SetTransform(const glm::mat4x4& transform);
    const glm::mat4x4& GetTransform() const;

private:
    glm::mat4x4 m_Transform;
};
DECLARE_SMART_PTR(Entity);

} // namespace WingsOfSteel::Pandora