#pragma once

#include <string>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "core/smart_ptr.hpp"

class btRigidBody;
class btMotionState;

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(CollisionShape);

enum class MotionType
{
    Static,
    Dynamic
};

class RigidBodyConstructionInfo
{
public:
    RigidBodyConstructionInfo() {}

    void SetMass(int32_t value) { m_Mass = value; }
    int32_t GetMass() const { return m_Mass; }

    void SetMotionType(MotionType motionType) { m_MotionType = motionType; }
    MotionType GetMotionType() const { return m_MotionType; }

    void SetWorldTransform(const glm::mat4x4& worldTransform) { m_WorldTransform = worldTransform; }
    const glm::mat4x4& GetWorldTransform() const { return m_WorldTransform; }

    void SetShape(CollisionShapeSharedPtr pShape) { m_pShape = pShape; }
    CollisionShapeSharedPtr GetShape() const { return m_pShape; }

    void SetLinearDamping(float value) { m_LinearDamping = value; }
    float GetLinearDamping() const { return m_LinearDamping; }

    void SetAngularDamping(float value) { m_AngularDamping = value; }
    float GetAngularDamping() const { return m_AngularDamping; }

    void SetFriction(float value) { m_Friction = value; }
    float GetFriction() const { return m_Friction; }

    void SetCentreOfMass(const glm::vec3& centreOfMass) { m_CentreOfMass = centreOfMass; }
    const glm::vec3& GetCentreOfMass() const { return m_CentreOfMass; }

private:
    int32_t m_Mass{1};
    MotionType m_MotionType{MotionType::Dynamic};
    glm::mat4x4 m_WorldTransform{1};
    CollisionShapeSharedPtr m_pShape;
    float m_LinearDamping{0.0f};
    float m_AngularDamping{0.0f};
    float m_Friction{0.0f};
    glm::vec3 m_CentreOfMass{0.0f, 0.0f, 0.0f};
};

class RigidBodyComponent
{
public:
    RigidBodyComponent(const RigidBodyConstructionInfo& ci);
    
    btRigidBody* GetBulletRigidBody() { return m_pRigidBody.get(); }
    const btRigidBody* GetBulletRigidBody() const { return m_pRigidBody.get(); }
    glm::mat4x4 GetWorldTransform() const;
    glm::vec3 GetPosition() const;
    glm::vec3 GetLinearVelocity() const;
    glm::vec3 GetAngularVelocity() const;
    const glm::mat3x3& GetInvInertiaTensorWorld() const;
    MotionType GetMotionType() const;
    int GetMass() const;
    const glm::vec3& GetCentreOfMass() const;
    void SetLinearDamping(float value);
    float GetLinearDamping() const;
    void SetAngularDamping(float value);
    float GetAngularDamping() const;

    void SetWorldTransform(const glm::mat4x4& worldTransform);
    void SetLinearVelocity(const glm::vec3& linearVelocity);
    void SetAngularVelocity(const glm::vec3& angularVelocity);
    void SetMotionType(MotionType motionType);

    void ApplyAngularForce(const glm::vec3& force);
    void ApplyLinearForce(const glm::vec3& force);
    void ApplyAngularImpulse(const glm::vec3& impulse);
    void ApplyLinearImpulse(const glm::vec3& impulse);

    void SetLinearFactor(const glm::vec3& linearFactor);
    const glm::vec3& GetLinearFactor() const;
    void SetAngularFactor(const glm::vec3& angularFactor);
    const glm::vec3& GetAngularFactor() const;

private:
    void CalculateInvInertiaTensorWorld();

    CollisionShapeSharedPtr m_pShape;
    std::unique_ptr<btRigidBody> m_pRigidBody;
    std::unique_ptr<btMotionState> m_pMotionState;
    MotionType m_MotionType{MotionType::Dynamic};
    int32_t m_Mass{1};
    float m_LinearDamping{0.0f};
    float m_AngularDamping{0.0f};
    glm::vec3 m_CentreOfMass{0.0f, 0.0f, 0.0f};
    glm::vec3 m_LinearFactor{0.0f, 0.0f, 0.0f};
    glm::vec3 m_AngularFactor{0.0f, 0.0f, 0.0f};
    glm::mat3x3 m_InvInertiaTensorWorld{1.0f};
};

} // namespace WingsOfSteel::Pandora