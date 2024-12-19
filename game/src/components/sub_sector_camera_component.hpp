#pragma once

#include <glm/vec3.hpp>

#include <core/smart_ptr.hpp>

namespace WingsOfSteel::Pandora
{
    DECLARE_SMART_PTR(Entity);
}

namespace WingsOfSteel::TheBrightestStar
{

class SubSectorCameraComponent
{
public:
    SubSectorCameraComponent() {}
    ~SubSectorCameraComponent() {}

    const glm::vec3& GetOffset() const;
    void SetOffset(const glm::vec3& offset);
    void SetOffset(float x, float y, float z);
    
    const glm::vec3& GetMaximumDrift() const;
    void SetMaximumDrift(const glm::vec3& maximumDrift);
    void SetMaximumDrift(float x, float y, float z);
    
    float GetDriftTimer() const;
    void SetDriftTimer(float driftTimer);
    
    const glm::vec3& GetTarget() const;
    void SetTarget(const glm::vec3& target);
    void SetTarget(float x, float y, float z);
    
    Pandora::EntityWeakPtr GetAnchorEntity() const;
    void SetAnchorEntity(const Pandora::EntityWeakPtr& anchorEntity);

private:
    glm::vec3 m_Offset{0.0f};
    glm::vec3 m_MaximumDrift{0.0f};
    float m_DriftTimer{0.0f};
    glm::vec3 m_Target{0.0f};
    Pandora::EntityWeakPtr m_AnchorEntity;
};

inline const glm::vec3& SubSectorCameraComponent::GetOffset() const
{
    return m_Offset;
}

inline void SubSectorCameraComponent::SetOffset(const glm::vec3& value)
{
    m_Offset = value;
}

inline void SubSectorCameraComponent::SetOffset(float x, float y, float z)
{
    m_Offset = glm::vec3(x, y, z);
}

inline const glm::vec3& SubSectorCameraComponent::GetMaximumDrift() const
{
    return m_MaximumDrift;
}

inline void SubSectorCameraComponent::SetMaximumDrift(const glm::vec3& value)
{
    m_MaximumDrift = value;
}

inline void SubSectorCameraComponent::SetMaximumDrift(float x, float y, float z)
{
    m_MaximumDrift = glm::vec3(x, y, z);
}

inline float SubSectorCameraComponent::GetDriftTimer() const
{
    return m_DriftTimer;
}

inline void SubSectorCameraComponent::SetDriftTimer(float value)
{
    m_DriftTimer = value;
}

inline const glm::vec3& SubSectorCameraComponent::GetTarget() const
{
    return m_Target;
}

inline void SubSectorCameraComponent::SetTarget(const glm::vec3& value)
{
    m_Target = value;
}

inline void SubSectorCameraComponent::SetTarget(float x, float y, float z)
{
    m_Target = glm::vec3(x, y, z);
}

inline Pandora::EntityWeakPtr SubSectorCameraComponent::GetAnchorEntity() const
{
    return m_AnchorEntity;
}

inline void SubSectorCameraComponent::SetAnchorEntity(const Pandora::EntityWeakPtr& value)
{
    m_AnchorEntity = value;
}

} // namespace WingsOfSteel::TheBrightestStar