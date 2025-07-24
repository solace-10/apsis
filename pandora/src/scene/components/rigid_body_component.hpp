#pragma once

#include <optional>
#include <string>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "core/smart_ptr.hpp"
#include "icomponent.hpp"
#include "component_factory.hpp"
#include "resources/resource_model.hpp"

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

REGISTER_COMPONENT(RigidBodyComponent, "rigid_body")
class RigidBodyComponent : public IComponent
{
public:
    RigidBodyComponent() = default;

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

    void SetLinearFactor(const glm::vec3& linearFactor);
    const glm::vec3& GetLinearFactor() const;
    void SetAngularFactor(const glm::vec3& angularFactor);
    const glm::vec3& GetAngularFactor() const;

    const glm::vec3 GetForwardVector() const;
    const glm::vec3 GetUpVector() const;
    const glm::vec3 GetRightVector() const;

    nlohmann::json Serialize() const override
    {
        nlohmann::json json;
        json["motion_type"] = static_cast<int>(m_MotionType);
        json["mass"] = m_Mass;
        json["linear_damping"] = m_LinearDamping;
        json["angular_damping"] = m_AngularDamping;
        json["linear_velocity"] = SerializeVec3(GetLinearVelocity());
        json["angular_velocity"] = SerializeVec3(GetAngularVelocity());

        return json;
    }

    void Deserialize(const nlohmann::json& json) override;

private:
    void CalculateInvInertiaTensorWorld();

    CollisionShapeSharedPtr m_pShape;
    std::unique_ptr<btRigidBody> m_pRigidBody;
    std::unique_ptr<btMotionState> m_pMotionState;
    MotionType m_MotionType{ MotionType::Dynamic };
    int32_t m_Mass{ 1 };
    float m_LinearDamping{ 0.0f };
    float m_AngularDamping{ 0.0f };
    glm::vec3 m_CentreOfMass{ 0.0f, 0.0f, 0.0f };
    glm::vec3 m_LinearFactor{ 0.0f, 0.0f, 0.0f };
    glm::vec3 m_AngularFactor{ 0.0f, 0.0f, 0.0f };
    glm::mat3x3 m_InvInertiaTensorWorld{ 1.0f };
    ResourceModelSharedPtr m_pResource;
    std::string m_ResourcePath;
    std::optional<glm::mat4> m_WorldTransform;
};

} // namespace WingsOfSteel::Pandora