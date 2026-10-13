#pragma once

#include <utility>
#include <vector>

#include <btBulletCollisionCommon.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel
{

DECLARE_SMART_PTR(CollisionShape);
DECLARE_SMART_PTR(PhysicsSimulation);
DECLARE_SMART_PTR(RigidBody);

using CollisionShapeContainer = std::vector<CollisionShapeSharedPtr>;

/////////////////////////////////////////////////////////////////////
// CollisionShape
/////////////////////////////////////////////////////////////////////

DECLARE_SMART_PTR(CollisionShape);
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
        Cylinder,
        Heightfield
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

DECLARE_SMART_PTR(CollisionShapeBox);
class CollisionShapeBox : public CollisionShape
{
public:
    CollisionShapeBox(float width, float height, float depth);

    virtual Type GetType() const override { return Type::Box; }
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeCompound
/////////////////////////////////////////////////////////////////////

DECLARE_SMART_PTR(CollisionShapeCompound);
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

DECLARE_SMART_PTR(CollisionShapeSphere);
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

DECLARE_SMART_PTR(CollisionShapeConvexHull);
class CollisionShapeConvexHull : public CollisionShape
{
public:
    CollisionShapeConvexHull(const ConvexHullVertices& vertices);

    virtual Type GetType() const override { return Type::ConvexHull; }
};

/////////////////////////////////////////////////////////////////////
// CollisionShapeCylinder
/////////////////////////////////////////////////////////////////////

DECLARE_SMART_PTR(CollisionShapeCylinder);
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

/////////////////////////////////////////////////////////////////////
// CollisionShapeHeightfield
/////////////////////////////////////////////////////////////////////

DECLARE_SMART_PTR(CollisionShapeHeightfield);
class CollisionShapeHeightfield : public CollisionShape
{
public:
    // Builds a btHeightfieldTerrainShape from a row-major grid of heights (x fastest,
    // width * length values). cellSize is the horizontal spacing between grid points and
    // must match the visual mesh so the collision surface lines up with what is rendered.
    //
    // Bullet does not copy the height data, so this shape owns it for its whole lifetime.
    // Bullet also vertically centres the heightfield around (minHeight + maxHeight) / 2, so
    // the owning rigid body must be translated up by GetVerticalOffset() to align with the
    // raw heights that were fed in.
    CollisionShapeHeightfield(uint32_t width, uint32_t length, std::vector<float> heights, float cellSize);

    virtual Type GetType() const override { return Type::Heightfield; }

    float GetVerticalOffset() const { return m_VerticalOffset; }

private:
    std::vector<float> m_Heights; // Must outlive m_pShape: btHeightfieldTerrainShape holds a raw pointer to it.
    float m_VerticalOffset{ 0.0f };
};

} // namespace WingsOfSteel