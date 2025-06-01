#pragma once

#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "core/smart_ptr.hpp"

class btCollisionShape;
class btBoxShape;
class btSphereShape;

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(CollisionShape);
DECLARE_SMART_PTR(CollisionShapeCompound);
DECLARE_SMART_PTR(PhysicsSimulation);
DECLARE_SMART_PTR(RigidBody);

using CollisionShapeContainer = std::vector<CollisionShapeSharedPtr>;

/////////////////////////////////////////////////////////////////////
// CollisionShape
/////////////////////////////////////////////////////////////////////

class CollisionShape
{
public:
    CollisionShape() {}
    virtual ~CollisionShape() {}

    enum class Type
    {
        Box,
        Compound,
        Sphere,
        ConvexHull,
        Cylinder
    };

    virtual Type GetType() const = 0;
    void SetUserData(void* pUserData);
    void* GetUserData() const;

    const btCollisionShape* GetBulletShape() const { return m_pShape.get(); }
    btCollisionShape* GetBulletShape() { return m_pShape.get(); }

protected:
    std::unique_ptr<btCollisionShape> m_pShape;
    void* m_pUserData{ nullptr };
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeBox
/////////////////////////////////////////////////////////////////////

class CollisionShapeBox : public CollisionShape
{
public:
    CollisionShapeBox(float width, float height, float depth);

    virtual Type GetType() const override { return Type::Box; }
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeCompound
/////////////////////////////////////////////////////////////////////

class CollisionShapeCompound : public CollisionShape
{
public:
    CollisionShapeCompound();

    void AddChildShape(CollisionShapeSharedPtr pShape, const glm::mat4x4& localTransform);
    void RemoveChildShape(CollisionShapeSharedPtr pShape);
    void RemoveChildShape(uint32_t index);
    CollisionShapeSharedPtr GetChildShape(uint32_t index) const;
    glm::mat4x4 GetChildTransform(uint32_t index) const;
    uint32_t GetChildrenCount() const;

    virtual Type GetType() const override { return Type::Compound; }

private:
    using ChildShapeContainer = std::vector<std::pair<CollisionShapeSharedPtr, glm::mat4x4>>;
    ChildShapeContainer m_ChildShapes;
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeSphere
/////////////////////////////////////////////////////////////////////

class CollisionShapeSphere : public CollisionShape
{
public:
    CollisionShapeSphere(float radius);

    virtual Type GetType() const override { return Type::Sphere; }
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeConvexHull
/////////////////////////////////////////////////////////////////////

using ConvexHullVertices = std::vector<glm::vec3>;

class CollisionShapeConvexHull : public CollisionShape
{
public:
    CollisionShapeConvexHull(const ConvexHullVertices& vertices);

    virtual Type GetType() const override { return Type::ConvexHull; }
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeCylinder
/////////////////////////////////////////////////////////////////////

class CollisionShapeCylinder : public CollisionShape
{
public:
    enum class Axis
    {
        X,
        Y,
        Z
    };

    CollisionShapeCylinder(Axis axis, float width, float height, float depth);

    virtual Type GetType() const override { return Type::Cylinder; }
};

} // namespace WingsOfSteel::Pandora