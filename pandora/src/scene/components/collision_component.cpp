#include "collision_component.hpp"

#include "core/serialization.hpp"
#include "pandora.hpp"
#include "physics/collision_shape.hpp"
#include "resources/resource_system.hpp"

namespace WingsOfSteel
{

void CollisionComponent::DeserializeShape(const ResourceDataStore* pContext, const Json::Data& jsonData, BuildCallback buildCallback)
{
    auto shapeDataResult = Json::DeserializeObject(pContext, jsonData, "shape");
    if (!shapeDataResult.has_value())
    {
        Log::Error() << pContext->GetPath() << ": Collision component needs to have a shape.";
        return;
    }

    const Json::Data& shapeData = shapeDataResult.value();
    CollisionShape::Type shapeType = Json::DeserializeEnum<CollisionShape::Type>(pContext, shapeData, "type", CollisionShape::Type::Sphere);

    if (shapeType == CollisionShape::Type::Sphere)
    {
        const float radius = Json::DeserializeFloat(pContext, shapeData, "radius", 1.0f);
        m_pShape = std::make_shared<CollisionShapeSphere>(radius);
        buildCallback();
    }
    else if (shapeType == CollisionShape::Type::Box)
    {
        const glm::vec3 dimensions = Json::DeserializeVec3(pContext, shapeData, "dimensions", glm::vec3(1.0f, 1.0f, 1.0f));
        m_pShape = std::make_shared<CollisionShapeBox>(dimensions.x, dimensions.y, dimensions.z);
        buildCallback();
    }
    else if (shapeType == CollisionShape::Type::Cylinder)
    {
        const CollisionShapeCylinder::Axis axis = Json::DeserializeEnum<CollisionShapeCylinder::Axis>(pContext, shapeData, "axis", CollisionShapeCylinder::Axis::Y);
        const glm::vec3 dimensions = Json::DeserializeVec3(pContext, shapeData, "dimensions", glm::vec3(1.0f, 1.0f, 1.0f));
        m_pShape = std::make_shared<CollisionShapeCylinder>(axis, dimensions.x, dimensions.y, dimensions.z);
        buildCallback();
    }
    else if (shapeType == CollisionShape::Type::ConvexHull)
    {
        const std::string& resourcePath = Json::DeserializeString(pContext, shapeData, "resource");
        GetResourceSystem()->RequestResource(resourcePath, [this, buildCallback](ResourceSharedPtr pResource) {
            m_pResource = std::dynamic_pointer_cast<ResourceModel>(pResource);
            m_pShape = m_pResource->GetCollisionShape();
            buildCallback();
        });
    }
    else
    {
        Log::Error() << pContext->GetPath() << ": Unsupported collision shape.";
        return;
    }
}

} // namespace WingsOfSteel
