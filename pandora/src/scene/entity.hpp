#pragma once

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <webgpu/webgpu_cpp.h>

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

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        return m_pScene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template<typename T>
    bool HasComponent() const
    {
        return m_pScene->m_Registry.try_get<T>(m_EntityHandle) != nullptr;
    }

    template<typename T>
    T& GetComponent()
    {
        return m_pScene->m_Registry.get<T>(m_EntityHandle);
    }

    virtual void OnAddedToScene();
    virtual void OnRemovedFromScene();
    virtual void Render(wgpu::RenderPassEncoder renderPass);

    void SetTransform(const glm::mat4x4& transform);
    const glm::mat4x4& GetTransform() const;

    friend Scene;

private:
    Scene* m_pScene = nullptr;
    entt::entity m_EntityHandle = entt::null;

    glm::mat4x4 m_Transform;
    bool m_MarkedForRemoval;
};

} // namespace WingsOfSteel::Pandora