#pragma once

#include <glm/mat4x4.hpp>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Entity);
DECLARE_SMART_PTR(Scene);

class Entity
{
public:
    Entity();
    virtual ~Entity();

    virtual void OnAddedToScene(Scene* pScene);
    virtual void OnRemovedFromScene();
    virtual void Update(float delta);
    virtual void Render();

    void SetTransform(const glm::mat4x4& transform);
    const glm::mat4x4& GetTransform() const;

    friend Scene;

private:
    Scene* m_pScene;
    glm::mat4x4 m_Transform;
    bool m_MarkedForRemoval;
};

} // namespace WingsOfSteel::Pandora