#include <btBulletCollisionCommon.h>
#include <glm/gtc/type_ptr.hpp>

#include "core/log.hpp"
#include "physics/collision_shape.hpp"

namespace WingsOfSteel::Pandora
{

/////////////////////////////////////////////////////////////////////
// CollisionShape
/////////////////////////////////////////////////////////////////////

void CollisionShape::SetUserData(void* pUserData)
{
	m_pUserData = pUserData;
}

void* CollisionShape::GetUserData() const
{
	return m_pUserData;
}


/////////////////////////////////////////////////////////////////////
// CollisionShapeBox
/////////////////////////////////////////////////////////////////////

CollisionShapeBox::CollisionShapeBox(float width, float height, float depth)
{
	m_pShape = std::make_unique<btBoxShape>(btVector3(width / 2.0f, height / 2.0f, depth / 2.0f));
	m_pShape->setUserPointer(this);
}


/////////////////////////////////////////////////////////////////////
// CollisionShapeCompound
/////////////////////////////////////////////////////////////////////

CollisionShapeCompound::CollisionShapeCompound()
{
	m_pShape = std::make_unique<btCompoundShape>(true, 4);
	m_pShape->setUserPointer( this );
	m_ChildShapes.reserve( 4 );
}

void CollisionShapeCompound::AddChildShape(CollisionShapeSharedPtr pShape, const glm::mat4x4& localTransform)
{
	btCompoundShape* pCompoundShape = static_cast<btCompoundShape*>(m_pShape.get());
	btTransform tr;
	tr.setFromOpenGLMatrix(glm::value_ptr(localTransform));
	pCompoundShape->addChildShape(tr, pShape->GetBulletShape());
	m_ChildShapes.push_back(std::pair<CollisionShapeSharedPtr, glm::mat4x4>(pShape, localTransform));
}

uint32_t CollisionShapeCompound::GetChildrenCount() const
{
	return static_cast<uint32_t>(m_ChildShapes.size());
}

// Removes the first instance of pShape from the CollisionShapeCompound's list of children.
void CollisionShapeCompound::RemoveChildShape(CollisionShapeSharedPtr pShape)
{
	const uint32_t numChildShapes = static_cast<uint32_t>(m_ChildShapes.size());
	for (uint32_t idx = 0; idx < numChildShapes; ++idx)
	{
		if (m_ChildShapes[idx].first == pShape)
		{
			RemoveChildShape(idx);
			break;
		}
	}
}

void CollisionShapeCompound::RemoveChildShape(uint32_t index)
{
	btCompoundShape* pCompoundShape = static_cast<btCompoundShape*>(m_pShape.get());
	pCompoundShape->removeChildShapeByIndex(index);

	// This needs to be a swap and pop to match Bullet's underlying implementation,
	// otherwise we end up with mismatched indices.
	std::swap(m_ChildShapes[index], m_ChildShapes[m_ChildShapes.size() - 1]);
	m_ChildShapes.pop_back();
}

CollisionShapeSharedPtr CollisionShapeCompound::GetChildShape(uint32_t index) const
{
	if (index >= m_ChildShapes.size())
	{
		Log::Error() << "Invalid access in CollisionShapeCompound::GetChildShape, accessing index " << index << " out of " << m_ChildShapes.size();
		return nullptr;
	}
	else
	{
		return m_ChildShapes[index].first;
	}
}

glm::mat4x4 CollisionShapeCompound::GetChildTransform(uint32_t index) const
{
	if (index >= m_ChildShapes.size())
	{
		Log::Error() << "Invalid access in CollisionShapeCompound::GetChildTransform, accessing index " << index << " out of " << m_ChildShapes.size();
		return glm::mat4x4(1.0f);
	}
	else
	{
		return m_ChildShapes[index].second;
	}
}


/////////////////////////////////////////////////////////////////////
// SphereShape
/////////////////////////////////////////////////////////////////////

CollisionShapeSphere::CollisionShapeSphere(float radius)
{
	m_pShape = std::make_unique<btSphereShape>(radius);
	m_pShape->setUserPointer(this);
}


/////////////////////////////////////////////////////////////////////
// ConvexHullShape
/////////////////////////////////////////////////////////////////////

CollisionShapeConvexHull::CollisionShapeConvexHull(const ConvexHullVertices& vertices)
{
	m_pShape = std::make_unique<btConvexHullShape>(reinterpret_cast<const btScalar*>(vertices.data()), static_cast<int>(vertices.size()), static_cast<int>(sizeof(glm::vec3)));
	m_pShape->setUserPointer(this);
}


/////////////////////////////////////////////////////////////////////
// CylinderShape
/////////////////////////////////////////////////////////////////////

CollisionShapeCylinder::CollisionShapeCylinder(Axis axis, float width, float height, float depth)
{
	btVector3 halfExtents(width / 2.0f, height / 2.0f, depth / 2.0f);
	if (axis == Axis::X)
	{
		m_pShape = std::make_unique<btCylinderShapeX>(halfExtents);
	}
	else if (axis == Axis::Y)
	{
		m_pShape = std::make_unique<btCylinderShape>(halfExtents);
	}
	else if (axis == Axis::Z)
	{
		m_pShape = std::make_unique<btCylinderShapeZ>(halfExtents);
	}
	m_pShape->setUserPointer(this);
}

} // namespace WingsOfSteel::Pandora