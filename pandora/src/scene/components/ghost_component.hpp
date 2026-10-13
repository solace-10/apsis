#pragma once

#include <optional>
#include <string>

#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "core/smart_ptr.hpp"
#include "collision_component.hpp"
#include "component_factory.hpp"
#include "scene/entity.hpp"
#include "resources/resource_model.hpp"

namespace WingsOfSteel
{

// Safe wrapper for storing Entity weak_ptr in Bullet's void* user pointer
struct GhostEntityUserData
{
    EntityWeakPtr entity;

    GhostEntityUserData(EntityWeakPtr pEntity) : entity(pEntity) {}
};

REGISTER_COMPONENT(GhostComponent, "ghost")
class GhostComponent : public CollisionComponent
{
public:
    GhostComponent() = default;
    ~GhostComponent();

    btGhostObject* GetBulletGhostObject() { return m_pGhostObject.get(); }
    const btGhostObject* GetBulletGhostObject() const { return m_pGhostObject.get(); }
    glm::mat4x4 GetWorldTransform() const;
    glm::vec3 GetPosition() const;

    void SetWorldTransform(const glm::mat4x4& worldTransform);

    const glm::vec3 GetForwardVector() const;
    const glm::vec3 GetUpVector() const;
    const glm::vec3 GetRightVector() const;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& jsonData) override;

    void SetOwner(EntitySharedPtr pOwner);
    EntityWeakPtr GetOwner() { return m_pOwner; }

    // Static utility function to safely get Entity from ghost object user pointer
    static EntitySharedPtr GetEntityFromGhostObject(const btGhostObject* pGhostObject);

private:
    void BuildGhostObject();

    std::unique_ptr<btGhostObject> m_pGhostObject;
    std::unique_ptr<GhostEntityUserData> m_pUserData;
};

} // namespace WingsOfSteel
