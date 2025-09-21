#pragma once

#include <entt/entt.hpp>

#include "core/smart_ptr.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Scene);

using EntityHandle = entt::entity;

DECLARE_SMART_PTR(Entity);
class Entity
{
public:
    Entity(Scene* pScene);
    virtual ~Entity();

    template <typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        return m_pScene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template <typename T>
    bool HasComponent() const
    {
        return m_pScene->m_Registry.try_get<T>(m_EntityHandle) != nullptr;
    }

    template <typename T>
    T& GetComponent()
    {
        return m_pScene->m_Registry.get<T>(m_EntityHandle);
    }

    EntityWeakPtr GetParent() { return m_pParentEntity; }
    void SetParent(EntityWeakPtr pParent) { m_pParentEntity = pParent; }

    virtual void OnAddedToScene();
    virtual void OnRemovedFromScene();

    friend Scene;

private:
    Scene* m_pScene{ nullptr };
    EntityHandle m_EntityHandle{ entt::null };
    EntityWeakPtr m_pParentEntity;
    bool m_MarkedForRemoval{ false };
};

} // namespace WingsOfSteel