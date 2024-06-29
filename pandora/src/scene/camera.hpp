#pragma once

#include <glm/vec3.hpp>

#include "scene/entity.hpp"

namespace WingsOfSteel::Pandora
{

class Camera : public Entity
{
public:
    Camera();
    ~Camera();

    void LookAt(const glm::vec3& cameraPosition, const glm::vec3& targetPosition, const glm::vec3& up);
    
private:
};

} // namespace WingsOfSteel::Pandora