#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(Entity);
DECLARE_SMART_PTR(Scene);
DECLARE_SMART_PTR(System);

using EntityHandle = entt::entity;

class Scene : public std::enable_shared_from_this<Scene>
{
public:
    Scene();
    ~Scene();

    SceneWeakPtr GetWeakPtr()
    {
        return weak_from_this();
    }

    virtual void Initialize();
    virtual void Update(float delta);

    EntitySharedPtr CreateEntity();
    void RemoveEntity(EntitySharedPtr pEntity);
    EntitySharedPtr GetEntity(EntityHandle handle);

    template <typename T>
    T* GetSystem() const
    {
        static_assert(std::is_base_of<System, T>::value, "T must inherit from System");
        for (const auto& pSystem : m_Systems)
        {
            if (auto* pTypedSystem = dynamic_cast<T*>(pSystem.get()))
            {
                return pTypedSystem;
            }
        }
        return nullptr;
    }

    template <typename T, typename... Args>
    T* AddSystem(Args&&... args)
    {
        static_assert(std::is_base_of<System, T>::value, "T must inherit from System");
        auto pSystem = std::make_unique<T>(std::forward<Args>(args)...);
        T* pTypedSystem = pSystem.get();
        pSystem->Initialize(this);
        m_Systems.push_back(std::move(pSystem));
        return pTypedSystem;
    }

    void SetCamera(EntitySharedPtr pCamera);
    EntitySharedPtr GetCamera() const;

    friend Entity;

    // Temp
    entt::registry& GetRegistry();

private:
    void ProcessEntitiesToAdd();
    void ProcessEntitiesToRemove();

    std::unordered_map<entt::entity, EntitySharedPtr> m_Entities;
    std::vector<EntitySharedPtr> m_EntitiesPendingAdd;
    EntityWeakPtr m_pCamera;
    entt::registry m_Registry;
    std::vector<SystemUniquePtr> m_Systems;
};

} // namespace WingsOfSteel