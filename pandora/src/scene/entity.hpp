#pragma once

#include <entt/entt.hpp>

#include "core/smart_ptr.hpp"
#include "scene/scene.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Scene);

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

    virtual void OnAddedToScene();
    virtual void OnRemovedFromScene();

    friend Scene;

private:
    Scene* m_pScene = nullptr;
    entt::entity m_EntityHandle = entt::null;
    bool m_MarkedForRemoval;
};

} // namespace WingsOfSteel::Pandora