#include <cassert>

#include <glm/gtc/type_ptr.hpp>

#include "core/serialization.hpp"
#include "pandora.hpp"
#include "physics/collision_shape.hpp"
#include "resources/resource_system.hpp"
#include "scene/components/ghost_component.hpp"

namespace WingsOfSteel
{

GhostComponent::~GhostComponent()
{
    if (m_pGhostObject)
    {
        m_pGhostObject->setUserPointer(nullptr);
    }
}

void GhostComponent::Deserialize(const ResourceDataStore* pContext, const Json::Data& jsonData)
{
    CollisionComponent::DeserializeShape(pContext, jsonData, [this]() {
        BuildGhostObject();
    });
}

void GhostComponent::BuildGhostObject()
{
    if (!m_pShape)
    {
        Log::Error() << "Ghost object can't be built without a shape.";
        return;
    }

    m_pGhostObject = std::make_unique<btGhostObject>();

    btTransform worldTransform;
    worldTransform.setFromOpenGLMatrix(glm::value_ptr(GetWorldTransform()));
    m_pGhostObject->setWorldTransform(worldTransform);

    m_pGhostObject->setCollisionShape(m_pShape->GetBulletShape());
    m_pGhostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
    m_pGhostObject->setUserPointer(this);
}

glm::vec3 GhostComponent::GetPosition() const
{
    if (m_pGhostObject)
    {
        const btTransform& tr = m_pGhostObject->getWorldTransform();
        const btVector3& position = tr.getOrigin();
        return glm::vec3(position.x(), position.y(), position.z());
    }
    else
    {
        return glm::vec3(0.0f);
    }
}

glm::mat4x4 GhostComponent::GetWorldTransform() const
{
    if (m_pGhostObject)
    {
        const btTransform& tr = m_pGhostObject->getWorldTransform();

        float mat[16];
        tr.getOpenGLMatrix(mat);

        return glm::make_mat4x4(mat);
    }
    else
    {
        return m_WorldTransform.value_or(glm::mat4(1.0f));
    }
}

void GhostComponent::SetWorldTransform(const glm::mat4x4& worldTransform)
{
    if (m_pGhostObject)
    {
        btTransform tr;
        tr.setFromOpenGLMatrix(glm::value_ptr(worldTransform));
        m_pGhostObject->setWorldTransform(tr);
    }
    else
    {
        m_WorldTransform = worldTransform;
    }
}

const glm::vec3 GhostComponent::GetForwardVector() const
{
    const btVector3 dir = m_pGhostObject->getWorldTransform().getBasis()[0];
    return glm::vec3(dir[2], dir[1], dir[0]);
}

const glm::vec3 GhostComponent::GetUpVector() const
{
    const btVector3 dir = m_pGhostObject->getWorldTransform().getBasis()[1];
    return glm::vec3(dir[2], dir[1], dir[0]);
}

const glm::vec3 GhostComponent::GetRightVector() const
{
    const btVector3 dir = m_pGhostObject->getWorldTransform().getBasis()[2];
    return glm::vec3(dir[2], dir[1], dir[0]);
}

void GhostComponent::SetOwner(EntitySharedPtr pOwner)
{
    assert(m_pGhostObject != nullptr);
    m_pOwner = pOwner;

    // We cannot set the GhostObject's user pointer to `this`, as components can get moved!
    m_pUserData = std::make_unique<GhostEntityUserData>(pOwner);
    m_pGhostObject->setUserPointer(m_pUserData.get());
}

EntitySharedPtr GhostComponent::GetEntityFromGhostObject(const btGhostObject* pGhostObject)
{
    if (!pGhostObject)
    {
        return nullptr;
    }

    GhostEntityUserData* pUserData = static_cast<GhostEntityUserData*>(pGhostObject->getUserPointer());
    if (!pUserData)
    {
        return nullptr;
    }

    return pUserData->entity.lock();
}

} // namespace WingsOfSteel
