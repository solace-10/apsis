#pragma once

#include <glm/vec3.hpp>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

class Camera : public Entity
{
public:
    Camera();
    Camera(float fov, float nearPlane, float farPlane);
    ~Camera();

    void LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up);
    
    void SetNearPlane(float distance);
    float GetNearPlane() const;
    void SetFarPlane(float distance);
    float GetFarPlane() const;
    void SetFieldOfView(float degrees);
    float GetFieldOfView() const;
    
    const glm::mat4& GetViewMatrix() const; // Same as GetTransform(), exists just for ergonomics.
    const glm::mat4& GetProjectionMatrix() const;

private:
    void CalculateProjectionMatrix();

    float m_Fov;
    float m_NearPlane;
    float m_FarPlane;
    glm::mat4 m_ProjectionMatrix;
};

} // namespace WingsOfSteel::Pandora