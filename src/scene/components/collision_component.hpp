#pragma once

#include <functional>
#include <optional>
#include <string>

#include <glm/mat4x4.hpp>

#include "core/smart_ptr.hpp"
#include "icomponent.hpp"
#include "scene/entity.hpp"
#include "resources/resource_model.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(CollisionShape);

// Base class for components that use Bullet collision shapes
class CollisionComponent : public IComponent
{
protected:
    CollisionComponent() = default;
    virtual ~CollisionComponent() = default;

    // Common shape deserialization with callback for building the specific collision object
    using BuildCallback = std::function<void()>;
    void DeserializeShape(const ResourceDataStore* pContext, const Json::Data& jsonData, BuildCallback buildCallback);

    // Common members
    CollisionShapeSharedPtr m_pShape;
    ResourceModelSharedPtr m_pResource;
    std::string m_ResourcePath;
    std::optional<glm::mat4> m_WorldTransform;
    EntityWeakPtr m_pOwner;
};

} // namespace WingsOfSteel
